#include "OperatorRepository.hpp"
#include "OperatorDAO.hpp"
#include "sqlite3.h"

#include <sstream>
#include <iostream>

//--------------------------------
//
//--------------------------------
OperatorRepository::OperatorRepository(const std::string& db_path) {
    set_db_path(db_path);
    ensure_table();
}

//--------------------------------
//
//--------------------------------
void OperatorRepository::ensure_table() {
    auto db = open_readwrite();

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS operators (
            operator_id INTEGER PRIMARY KEY AUTOINCREMENT,
            operator_uid TEXT UNIQUE NOT NULL,
            username TEXT NOT NULL,
            password TEXT NOT NULL,
            clearance INTEGER NOT NULL
        );
    )");
}

//--------------------------------
//
//--------------------------------
std::vector<OperatorDAO> OperatorRepository::list() const {
    auto db = open_readonly();

    std::vector<OperatorDAO> results;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "SELECT operator_id, operator_uid, username, password, clearance FROM operators;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed: " + std::string(sqlite3_errmsg(h)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        OperatorDAO op;

        op.id = sqlite3_column_int64(stmt, 0);

        const char* uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        op.uid = uid_ptr ? uid_ptr : "";

        const char* user_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        op.username = user_ptr ? user_ptr : "";

        const char* pass_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        op.password = pass_ptr ? pass_ptr : "";

        op.clearance = sqlite3_column_int(stmt, 4);

        results.push_back(std::move(op));
    }

    sqlite3_finalize(stmt);
    return results;
}

//--------------------------------
//
//--------------------------------
std::optional<OperatorDAO> OperatorRepository::get(int64_t id) const {
    auto db = open_readonly();

    std::optional<OperatorDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT operator_id, operator_uid, username, password, clearance FROM operators WHERE operator_id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        OperatorDAO op;
        op.id = sqlite3_column_int64(stmt, 0);
        op.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        op.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        op.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        op.clearance = sqlite3_column_int(stmt, 4);
        opt = op;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
std::optional<OperatorDAO> OperatorRepository::get(const UUID& uid) const {
    auto db = open_readonly();
    std::optional<OperatorDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT operator_id, operator_uid, username, password, clearance FROM operators WHERE operator_uid = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        OperatorDAO op;
        op.id = sqlite3_column_int64(stmt, 0);
        op.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        op.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        op.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        op.clearance = sqlite3_column_int(stmt, 4);
        opt = op;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
std::optional<OperatorDAO> OperatorRepository::get_username(const std::string& username) {
    auto db = open_readonly();
    std::optional<OperatorDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT operator_id, operator_uid, username, password, clearance FROM operators WHERE username = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        OperatorDAO op;
        op.id = sqlite3_column_int64(stmt, 0);
        op.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        op.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        op.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        op.clearance = sqlite3_column_int(stmt, 4);
        opt = op;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
OperatorDAO OperatorRepository::create(const OperatorDAO& op) {
    auto db = open_readwrite();
    sqlite3* h = db.getHandle();
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "INSERT INTO operators (operator_uid, username, password, clearance) "
                         "VALUES (?, ?, ?, ?);";

        if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw SqliteException("prepare failed");
        }

        UUID uid = op.uid.empty() ? generate_uuid() : op.uid;

        sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, op.username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, op.password.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, op.clearance);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw SqliteException("insert failed");
        }

        sqlite3_finalize(stmt);
        int64_t id = sqlite3_last_insert_rowid(h);

        OperatorDAO result = op;
        result.id = id;
        result.uid = uid;
        return result;
}

//--------------------------------
//
//--------------------------------
std::optional<OperatorDAO> OperatorRepository::update(int64_t id, const OperatorDAO& op) {
    auto db = open_readwrite();
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE operators SET username = ?, password = ?, clearance = ? "
                        "WHERE operator_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, op.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, op.password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, op.clearance);
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
std::optional<OperatorDAO> OperatorRepository::upsert(const std::string& username, const std::string& password) {
    //TODO: this is not a working implementation right now!!!!
    std::cout << ("OperatorRepository::upsert was called! This shouldn't be used yet!");
    std::terminate();

    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO operators (username, password) VALUES (?, ?) "
                      "ON CONFLICT(username) DO UPDATE SET password = excluded.password;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    sqlite3_finalize(stmt);
    return get(username);
}

//--------------------------------
//
//--------------------------------
bool OperatorRepository::remove(int64_t id) {
    auto db = open_readwrite();
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM operators WHERE operator_id = ?;";

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
bool OperatorRepository::remove(const std::string& username) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM operators WHERE username = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

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
void OperatorRepository::commit() {

}