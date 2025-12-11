#pragma once
#include <string>
#include <chrono>
#include <vector>
#include <optional>

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;

struct User {
    long long id{0};
    std::string uid; // uuid string
    std::string username;
    std::string password_hash;
    int clearance{0};
};

struct VictimTemplate {
    long long id{0};
    std::string uid;
    std::string username;
    std::string password;
};

struct Victim {
    long long id{0};
    std::string uid;
    std::string internal_ip;
    std::string external_ip;
    std::string hostname;
    std::string username;
    std::string operating_system;
    TimePoint last_update{Clock::now()};
    int status{0};
    std::optional<long long> template_id;
};

struct Command {
    long long id{0};
    std::string uid;
    long long prev{0};
    long long nonce{0};
    std::string command;
    std::string signature;
    int status{0};
};

struct Result {
    long long id{0};
    std::string uid;
    std::string data;
};

struct Session {
    long long id{0};
    std::string uid;
    int port{0};
};

struct Log {
    long long id{0};
    std::string uid;
    std::string key;
    std::string value;
    TimePoint time{Clock::now()};
};