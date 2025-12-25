#pragma once

#include <string>
#include <vector>

class LogManager {
public:
    // Delete copy and move constructors/assignments (singleton pattern)
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    LogManager(LogManager&&) = delete;
    LogManager& operator=(LogManager&&) = delete;
    explicit LogManager() = default;

    static LogManager& instance();

    void local_log(const std::string& log);
    void attack_log(const std::string& log);
    std::vector<std::string> get_logs();
    std::vector<std::string> refresh();

private:
    std::vector<std::string> logs;
};