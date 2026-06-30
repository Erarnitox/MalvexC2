#include "ResultRepository.hpp"
#include "ResultDAO.hpp"
#include "sqlite3.h"

ResultRepository::ResultRepository(const std::string& db_path) {
    set_db_path(db_path);
    ensure_table();
}

void ResultRepository::ensure_table() {
    auto db = open_readwrite();

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS results (
            result_id INTEGER PRIMARY KEY AUTOINCREMENT,
            result_uid TEXT UNIQUE NOT NULL,
            command_uid TEXT NOT NULL DEFAULT '',
            data TEXT NOT NULL
        );
    )");

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* alter_sql = "ALTER TABLE results ADD COLUMN command_uid TEXT NOT NULL DEFAULT '';";
    if (sqlite3_prepare_v2(h, alter_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

std::vector<ResultDAO> ResultRepository::list() const {
    auto db = open_readonly();

    std::vector<ResultDAO> results;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "SELECT result_id, result_uid, command_uid, data FROM results;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed: " + std::string(sqlite3_errmsg(h)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ResultDAO result;
        result.id = sqlite3_column_int64(stmt, 0);

        const char* uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.uid = uid_ptr ? uid_ptr : "";

        const char* command_uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.command_uid = command_uid_ptr ? command_uid_ptr : "";

        const char* data_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        result.data = data_ptr ? data_ptr : "";

        results.push_back(std::move(result));
    }

    sqlite3_finalize(stmt);
    return results;
}

std::optional<ResultDAO> ResultRepository::get(int64_t id) const {
    auto db = open_readonly();

    std::optional<ResultDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT result_id, result_uid, command_uid, data FROM results WHERE result_id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ResultDAO result;
        result.id = sqlite3_column_int64(stmt, 0);
        result.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.command_uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        opt = result;
    }

    sqlite3_finalize(stmt);
    return opt;
}

std::optional<ResultDAO> ResultRepository::get(const UUID& uid) const {
    auto db = open_readonly();

    std::optional<ResultDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT result_id, result_uid, command_uid, data FROM results WHERE result_uid = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ResultDAO result;
        result.id = sqlite3_column_int64(stmt, 0);
        result.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.command_uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        opt = result;
    }

    sqlite3_finalize(stmt);
    return opt;
}

ResultDAO ResultRepository::create(const ResultDAO& result) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO results (result_uid, command_uid, data) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    UUID uid = result.uid.empty() ? generate_uuid() : result.uid;

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, result.command_uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, result.data.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        const std::string err = sqlite3_errmsg(h);
        sqlite3_finalize(stmt);
        throw SqliteException("insert failed: " + err);
    }

    sqlite3_finalize(stmt);
    int64_t id = sqlite3_last_insert_rowid(h);

    ResultDAO created = result;
    created.id = id;
    created.uid = uid;
    created.command_uid = result.command_uid;
    return created;
}

std::optional<ResultDAO> ResultRepository::update(int64_t id, const ResultDAO& result) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE results SET data = ? WHERE result_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, result.data.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    sqlite3_finalize(stmt);
    return get(id);
}

bool ResultRepository::remove(int64_t id) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM results WHERE result_id = ?;";

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

void ResultRepository::commit() {
}
