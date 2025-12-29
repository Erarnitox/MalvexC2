#pragma once

#include "SessionDAO.hpp"

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

private:
    std::vector<SessionDAO> m_sessions;
};