#pragma once

#include "LogDAO.hpp"
#include <RESTClient.hpp>
#include <mutex>
#include <string>
#include <vector>

class LogManager {
public:
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
    std::vector<LogDAO> refresh_send_buffer();

    void set_list(const std::vector<LogDAO>& log_list) noexcept;

private:
    mutable std::mutex m_mtx;
    std::vector<std::string> m_local_logs;
    std::map<UUID, LogDAO> m_attack_logs;
    std::vector<LogDAO> m_send_buffer;
};