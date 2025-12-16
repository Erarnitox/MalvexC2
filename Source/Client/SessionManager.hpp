#pragma once

class SessionManager {
public:
    // Delete copy and move constructors/assignments (singleton pattern)
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    SessionManager(SessionManager&&) = delete;
    SessionManager& operator=(SessionManager&&) = delete;
    explicit SessionManager() = default;

    static SessionManager& instance();

};