#include "LogRepository.hpp"
#include "LogDAO.hpp"
#include "Types.hpp"
#include "sqlite3.h"

#include <cstdint>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>

//--------------------------------
//
//--------------------------------
LogRepository::LogRepository(const std::string& db_path) {
    set_db_path(db_path);
    ensure_table();
}

//--------------------------------
//
//--------------------------------
void LogRepository::ensure_table() {
    auto db = open_readwrite();

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS logs (
            log_id INTEGER PRIMARY KEY AUTOINCREMENT,
            log_uid TEXT UNIQUE NOT NULL,
            key TEXT NOT NULL,
            value TEXT NOT NULL,
            time TEXT NOT NULL
        );
    )");
}

//--------------------------------
//
//--------------------------------
std::vector<LogDAO> LogRepository::list() const {
    auto db = open_readonly();

    std::vector<LogDAO> results;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "SELECT log_id, log_uid, key, value, time FROM logs ORDER BY time DESC;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed: " + std::string(sqlite3_errmsg(h)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LogDAO log;

        log.id = sqlite3_column_int64(stmt, 0);

        const char* uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        log.uid = uid_ptr ? uid_ptr : "";

        const char* key_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        log.key = key_ptr ? key_ptr : "";

        const char* value_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        log.value = value_ptr ? value_ptr : "";

        TimePoint time = sqlite3_column_int64(stmt, 4);
        log.time = time;

        results.push_back(std::move(log));
    }

    sqlite3_finalize(stmt);
    return results;
}

//--------------------------------
//
//--------------------------------
std::vector<LogDAO> LogRepository::list_recent(int limit) {
    auto db = open_readonly();

    std::vector<LogDAO> results;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "SELECT log_id, log_uid, key, value, time FROM logs "
                      "ORDER BY time DESC LIMIT ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed: " + std::string(sqlite3_errmsg(h)));
    }

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LogDAO log;
        log.id = sqlite3_column_int64(stmt, 0);
        log.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        log.key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        log.value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        log.time = sqlite3_column_int64(stmt, 4);

        results.push_back(std::move(log));
    }

    sqlite3_finalize(stmt);
    return results;
}

//--------------------------------
//
//--------------------------------
std::optional<LogDAO> LogRepository::get(int64_t id) const {
    auto db = open_readonly();

    std::optional<LogDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT log_id, log_uid, key, value, time "
                      "FROM logs WHERE log_id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        LogDAO log;
        log.id = sqlite3_column_int64(stmt, 0);
        log.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        log.key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        log.value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        log.time = sqlite3_column_int64(stmt, 4);
        opt = log;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
std::optional<LogDAO> LogRepository::get(const UUID& uid) const {
    auto db = open_readonly();

    std::optional<LogDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT log_id, log_uid, key, value, time "
                      "FROM logs WHERE log_uid = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        LogDAO log;
        log.id = sqlite3_column_int64(stmt, 0);
        log.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        log.key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        log.value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        log.time = sqlite3_column_int64(stmt, 4);
        opt = log;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
LogDAO LogRepository::create(const LogDAO& log) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO logs (log_uid, key, value, time) VALUES (?, ?, ?, ?);";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    UUID uid = log.uid.empty() ? generate_uuid() : log.uid;
    TimePoint timestamp = log.time;

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, log.key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, log.value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, timestamp);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw SqliteException("insert failed");
    }

    sqlite3_finalize(stmt);
    int64_t id = sqlite3_last_insert_rowid(h);

    LogDAO result = log;
    result.id = id;
    result.uid = uid;
    return result;
}

//--------------------------------
//
//--------------------------------
std::optional<LogDAO> LogRepository::update(int64_t id, const LogDAO& log) {
    auto db = open_readwrite();
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE logs SET key = ?, value = ?, time = ? WHERE log_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    TimePoint timestamp = log.time;

    sqlite3_bind_text(stmt, 1, log.key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, log.value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, timestamp);
    sqlite3_bind_int64(stmt, 4, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    sqlite3_finalize(stmt);
    return get(id);
}

//--------------------------------
//
//--------------------------------
bool LogRepository::remove(int64_t id) {
    auto db = open_readwrite();
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM logs WHERE log_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return sqlite3_changes(h) > 0;
}

//--------------------------------
//
//--------------------------------
void LogRepository::commit() {

}