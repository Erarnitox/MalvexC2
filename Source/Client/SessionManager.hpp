#pragma once

#include "SessionDAO.hpp"

#include <atomic>
#include <cpppwn.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

inline constexpr std::size_t kMaxBridgeOutputBytes = 16 * 1024 * 1024;

struct SessionTransferResult {
    bool success = false;
    std::string message;
    std::string data;
};

enum class SessionBridgeState {
    Connecting,
    WaitingForVictim,
    Ready,
    Failed,
    Closed
};

class SessionManager;

//-------------------------------------------------
//
//-------------------------------------------------
class SessionConnection {
private:
    std::unique_ptr<cpppwn::Remote> conn;
    std::string session_id;
    std::atomic<SessionBridgeState> bridge_state{SessionBridgeState::Connecting};
    std::atomic<bool> running{true};
    std::mutex conn_mutex;
    std::thread worker;

    void connect_and_handshake(
        std::string host,
        int port,
        std::string username,
        std::string password);

public:
    SessionConnection(
        std::string host,
        int port,
        std::string sid,
        std::string username,
        std::string password);
    ~SessionConnection();

    void discard_pending_output();
    void discard_pending_output_unlocked();
    [[nodiscard]] std::string execute_cmd(const std::string& cmd);
    [[nodiscard]] SessionTransferResult download_file(const std::string& remote_filename);
    [[nodiscard]] SessionTransferResult upload_file(const std::string& remote_filename, const std::string& data);
    [[nodiscard]] std::string get_uuid() const;
    [[nodiscard]] SessionBridgeState get_bridge_state() const noexcept;
    void stop();
};

//-------------------------------------------------
//
//-------------------------------------------------
class SessionManager {
public:
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    SessionManager(SessionManager&&) = delete;
    SessionManager& operator=(SessionManager&&) = delete;
    explicit SessionManager() = default;

    static SessionManager& instance();

    const std::vector<SessionDAO>& getSessions() const noexcept;
    const SessionDAO getSession(int64_t id) const noexcept;
    void setList(std::vector<SessionDAO>&& session_list) noexcept;
    void startSession(
        const std::string& host,
        int64_t port,
        const std::string& username,
        const std::string& password);
    void close(const UUID& session_id);
    void discardPendingOutput(const SessionDAO& session);
    std::string execute(const SessionDAO& session, const std::string& cmd);
    [[nodiscard]] SessionTransferResult downloadFile(const SessionDAO& session, const std::string& remote_filename);
    [[nodiscard]] SessionTransferResult uploadFile(
        const SessionDAO& session,
        const std::string& remote_filename,
        const std::string& data);
    [[nodiscard]] SessionBridgeState getBridgeState(const SessionDAO& session) const noexcept;

private:
    mutable std::mutex m_mtx;
    std::vector<SessionDAO> m_sessions;
    std::vector<std::unique_ptr<SessionConnection>> m_connections;
    std::map<int64_t, UUID> m_port_to_connection;
};
