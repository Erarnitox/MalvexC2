#include "LogManager.hpp"
#include "LogDAO.hpp"
#include "Types.hpp"

#include <format>
#include <chrono>


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
    m_local_logs.push_back(std::format("[LOCAL]@[{}]: \"{}\"", timestamp, log));
}

//-------------------------------------------------
//
//-------------------------------------------------
void LogManager::attack_log(const std::string& log) {
    std::string timestamp = get_time();

    LogDAO log_entry;
    log_entry.key = "ATTACK";
    log_entry.time = get_unix_time();
    log_entry.uid = generate_uuid();
    log_entry.value = log;

    m_send_buffer.push_back(log_entry);
    m_attack_logs[log_entry.uid] = log_entry;
}

//-------------------------------------------------
//
//-------------------------------------------------
std::vector<std::string> LogManager::get_logs() {
    return m_local_logs;
}

//-------------------------------------------------
//
//-------------------------------------------------
std::vector<std::string> LogManager::refresh() {
    const auto res = m_local_logs;
    m_local_logs.clear();
    return res;
}

//-------------------------------------------------
//
//-------------------------------------------------
std::vector<LogDAO> LogManager::refresh_send_buffer() {
    const auto res = m_send_buffer;
    m_send_buffer.clear();
    return res;
}

//-------------------------------------------------
//
//-------------------------------------------------
void LogManager::set_list(const std::vector<LogDAO>& log_list) noexcept {
    for(const auto& log_entry : log_list) {
        if (not m_attack_logs.contains(log_entry.uid)) {
            m_attack_logs[log_entry.uid] = log_entry;

            // add the log entry to the local logs
            const auto timestamp = from_unix_time(log_entry.time);
            m_local_logs.push_back(std::format("[ATTACK]@[{}]: \"{}\"", timestamp, log_entry.value));
        }
    }
}
