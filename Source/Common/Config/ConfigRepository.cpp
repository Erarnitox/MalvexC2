#include "ConfigRepository.hpp"
#include "ConfigDAO.hpp"

#include <sstream>
#include <iostream>

//--------------------------------
//
//--------------------------------
ConfigRepository::ConfigRepository(const std::string& db_path)
    : db_(std::make_unique<Database>(db_path)) {
    ensure_table();
}

//--------------------------------
//
//--------------------------------
void ConfigRepository::ensure_table() {
    db_->exec(R"(
        CREATE TABLE IF NOT EXISTS config (
            config_id INTEGER PRIMARY KEY AUTOINCREMENT,
            key TEXT NOT NULL UNIQUE,
            value TEXT NOT NULL
        );
    )");
}

//--------------------------------
//
//--------------------------------
std::vector<ConfigDAO> ConfigRepository::list() {
    std::vector<ConfigDAO> result;
    db_->query("SELECT config_id, key, value FROM config ORDER BY key;",
        [&](int cols, char** values, char** names) {
            ConfigDAO c;
            c.id = values[0] ? std::stoll(values[0]) : 0;
            c.key = values[1] ? values[1] : "";
            c.value = values[2] ? values[2] : "";
            result.push_back(std::move(c));
        });
    return result;
}

//--------------------------------
//
//--------------------------------
std::optional<ConfigDAO> ConfigRepository::get(int64_t config_id) {
    std::optional<ConfigDAO> opt;
    std::ostringstream sql;
    sql << "SELECT config_id, key, value FROM config WHERE config_id = "
        << config_id << " LIMIT 1;";

    db_->query(sql.str(), [&](int cols, char** values, char** names) {
        if (cols >= 3) {
            ConfigDAO c;
            c.id = values[0] ? std::stoll(values[0]) : 0;
            c.key = values[1] ? values[1] : "";
            c.value = values[2] ? values[2] : "";
            opt = c;
        }
    });
    return opt;
}

//--------------------------------
//
//--------------------------------
std::optional<ConfigDAO> ConfigRepository::get(const std::string& key) {
    std::optional<ConfigDAO> opt;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT config_id, key, value FROM config WHERE key = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ConfigDAO c;
        c.id = sqlite3_column_int64(stmt, 0);
        c.key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        c.value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        opt = c;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
ConfigDAO ConfigRepository::create(const ConfigDAO& config) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO config (key, value) VALUES (?, ?);";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, config.key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, config.value.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw SqliteException("insert step failed");
    }

    sqlite3_finalize(stmt);
    int64_t id = sqlite3_last_insert_rowid(h);

    ConfigDAO copy = config;
    copy.id = id;
    return copy;
}

//--------------------------------
//
//--------------------------------
std::optional<ConfigDAO> ConfigRepository::update(int64_t id, const ConfigDAO& config) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE config SET key = ?, value = ? WHERE config_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, config.key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, config.value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, id);

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
std::optional<ConfigDAO> ConfigRepository::upsert(const std::string& key, const std::string& value) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO config (key, value) VALUES (?, ?) "
                      "ON CONFLICT(key) DO UPDATE SET value = excluded.value;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    sqlite3_finalize(stmt);
    return get(key);
}

//--------------------------------
//
//--------------------------------
bool ConfigRepository::remove(int64_t config_id) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM config WHERE config_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, config_id);

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
bool ConfigRepository::remove(const std::string& key) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM config WHERE key = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return sqlite3_changes(h) > 0;
}