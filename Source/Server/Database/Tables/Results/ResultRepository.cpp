#include "ResultRepository.hpp"
#include "ResultDAO.hpp"

#include <sstream>
#include <iostream>

//--------------------------------
//
//--------------------------------
ResultRepository::ResultRepository(const std::string& db_path)
    : db_(std::make_unique<Database>(db_path)) {
    ensure_table();
}

//--------------------------------
//
//--------------------------------
void ResultRepository::ensure_table() {
    db_->exec(R"(
        CREATE TABLE IF NOT EXISTS results (
            result_id INTEGER PRIMARY KEY AUTOINCREMENT,
            result_uid TEXT UNIQUE NOT NULL,
            data TEXT NOT NULL
        );
    )");
}

//--------------------------------
//
//--------------------------------
std::vector<ResultDAO> ResultRepository::list() {
    std::vector<ResultDAO> results;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "SELECT result_id, result_uid, data FROM results;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed: " + std::string(sqlite3_errmsg(h)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ResultDAO result;

        result.id = sqlite3_column_int64(stmt, 0);

        const char* uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.uid = uid_ptr ? uid_ptr : "";

        const char* data_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.data = data_ptr ? data_ptr : "";

        results.push_back(std::move(result));
    }

    sqlite3_finalize(stmt);
    return results;
}

//--------------------------------
//
//--------------------------------
std::optional<ResultDAO> ResultRepository::get(int64_t id) {
    std::optional<ResultDAO> opt;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT result_id, result_uid, data FROM results WHERE result_id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ResultDAO result;
        result.id = sqlite3_column_int64(stmt, 0);
        result.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        opt = result;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
ResultDAO ResultRepository::create(const ResultDAO& result) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO results (result_uid, data) VALUES (?, ?);";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    UUID uid = result.uid.empty() ? generate_uuid() : result.uid;

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, result.data.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw SqliteException("insert failed");
    }

    sqlite3_finalize(stmt);
    int64_t id = sqlite3_last_insert_rowid(h);

    ResultDAO created = result;
    created.id = id;
    created.uid = uid;
    return created;
}

//--------------------------------
//
//--------------------------------
std::optional<ResultDAO> ResultRepository::update(int64_t id, const ResultDAO& result) {
    sqlite3* h = db_->handle();
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

//--------------------------------
//
//--------------------------------
bool ResultRepository::remove(int64_t id) {
    sqlite3* h = db_->handle();
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

//--------------------------------
//
//--------------------------------
void ResultRepository::commit() {
    db_->commit();
}