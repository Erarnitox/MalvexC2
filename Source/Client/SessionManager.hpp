#pragma once

#include "SessionDAO.hpp"

#include <cpppwn.hpp>
#include <memory>

class SessionManager;

//-------------------------------------------------
//
//-------------------------------------------------
class SessionConnection {
private:
    std::unique_ptr<cpppwn::Remote> conn;
    std::string session_id;
    std::atomic<bool> running{true};

public:
    SessionConnection(std::string host, int port, std::string sid, std::string username, std::string password);
    [[nodiscard]] std::string execute_cmd(const std::string& cmd);
    [[nodiscard]] std::string get_uuid() const;
    void stop();
};

//-------------------------------------------------
//
//-------------------------------------------------
class SessionManager {
public:
    // Delete copy and move constructors/assignments (singleton pattern)
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
    std::string execute(const SessionDAO& session, const std::string& cmd);

private:
    std::vector<SessionDAO> m_sessions;
    std::vector<std::unique_ptr<SessionConnection>> m_connections;
    std::map<int64_t, UUID> m_port_to_connection;
};