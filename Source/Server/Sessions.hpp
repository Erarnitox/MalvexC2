#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <queue>

//TODO: encapsulate later
using SafeQueue = std::queue<std::string>;

//-------------------------------------------------
//
//-------------------------------------------------
class Session {
public:
    Session(uint16_t port, std::string victim_uid);
    void start(); // Spins up the listening thread
    void stop();
    void push_to_victim(const std::string& data);
    void push_to_attacker(const std::string& data);

private:
    uint16_t listen_port;
    std::string victim_uid;

    // Thread-safe queues for duplex communication
    SafeQueue attacker_to_victim;
    SafeQueue victim_to_attacker;
};

//-------------------------------------------------
//
//-------------------------------------------------
class SessionManager {
public:
    // Factory method to create and track sessions
    uint16_t create_session(const std::string& victim_uid) {
        uint16_t port = find_free_port();
        auto session = std::make_shared<Session>(port, victim_uid);
        sessions[port] = session;
        session->start();
        return port;
    }
private:
    std::unordered_map<uint16_t, std::shared_ptr<Session>> sessions;
};