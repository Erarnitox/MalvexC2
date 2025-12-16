#pragma once

class LogManager {
public:
    // Delete copy and move constructors/assignments (singleton pattern)
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    LogManager(LogManager&&) = delete;
    LogManager& operator=(LogManager&&) = delete;
    explicit LogManager() = default;

    static LogManager& instance();

};