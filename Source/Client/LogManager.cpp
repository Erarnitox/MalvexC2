#include "LogManager.hpp"

#include <format>
#include <chrono>

//-------------------------------------------------
//
//-------------------------------------------------
static inline
std::string get_time() {
    auto now = std::chrono::system_clock::now();
    return std::format("{:%Y-%m-%d %H:%M:%S}", now);
}

//-------------------------------------------------
//
//-------------------------------------------------
LogManager& LogManager::instance() {
    static LogManager instance;
    return instance;
}

//-------------------------------------------------
//
//-------------------------------------------------
void LogManager::local_log(const std::string& log) {
    std::string timestamp = get_time();
    logs.push_back(std::format("[LOCAL]@[{}]: \"{}\"", timestamp, log));
}

//-------------------------------------------------
//
//-------------------------------------------------
void LogManager::attack_log(const std::string& log) {
    std::string timestamp = get_time();
    logs.push_back(std::format("[ATTACK]@[{}]: \"{}\"", timestamp, log));
}

//-------------------------------------------------
//
//-------------------------------------------------
std::vector<std::string> LogManager::get_logs() {
    return logs;
}

//-------------------------------------------------
//
//-------------------------------------------------
std::vector<std::string> LogManager::refresh() {
    const auto res = logs;
    logs.clear();
    return res;
}
