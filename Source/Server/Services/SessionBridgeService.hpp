#pragma once

class SessionManager;

class SessionBridgeService {
public:
    explicit SessionBridgeService(SessionManager& sessions) : sessions_(sessions) {}

    bool start_listener(uint16_t port) { return sessions_.start_listener(port); }
    void stop_listener(uint16_t port) { sessions_.stop_listener(port); }
    void stop_all() { sessions_.stop_all(); }

private:
    SessionManager& sessions_;
};
