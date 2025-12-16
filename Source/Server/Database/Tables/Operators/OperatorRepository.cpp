#include "OperatorRepository.hpp"
#include "OperatorDAO.hpp"

#include <sstream>
#include <iostream>

//--------------------------------
//
//--------------------------------
OperatorRepository::OperatorRepository(const std::string& db_path)
    : db_(std::make_unique<Database>(db_path)) {
    ensure_table();
}

//--------------------------------
//
//--------------------------------
void OperatorRepository::ensure_table() {
    db_->exec(R"(
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
std::vector<OperatorDAO> OperatorRepository::list() {
   std::vector<OperatorDAO> result;
    db_->query("SELECT operator_id, operator_uid, username, password, clearance FROM operators;",
        [&](int cols, char** values, char** names) {
            OperatorDAO op;
            op.operator_id = values[0] ? std::stoll(values[0]) : 0;
            op.operator_uid = values[1] ? values[1] : "";
            op.username = values[2] ? values[2] : "";
            op.password = values[3] ? values[3] : "";
            op.clearance = values[4] ? std::stoi(values[4]) : 0;
            result.push_back(std::move(op));
        });
    return result;
}

//--------------------------------
//
//--------------------------------
std::optional<OperatorDAO> OperatorRepository::get(int64_t id) {
    std::optional<OperatorDAO> opt;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT operator_id, operator_uid, username, password, clearance "
                        "FROM operators WHERE operator_id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        OperatorDAO op;
        op.operator_id = sqlite3_column_int64(stmt, 0);
        op.operator_uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
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
std::optional<OperatorDAO> OperatorRepository::get(const std::string& username) {
    std::optional<OperatorDAO> opt;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT operator_id, operator_uid, username, password, clearance FROM operators WHERE username = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        OperatorDAO op;
        op.operator_id = sqlite3_column_int64(stmt, 0);
        op.operator_uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
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
    sqlite3* h = db_->handle();
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "INSERT INTO operators (operator_uid, username, password, clearance) "
                         "VALUES (?, ?, ?, ?);";

        if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw SqliteException("prepare failed");
        }

        UUID uid = op.operator_uid.empty() ? generate_uuid() : op.operator_uid;

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
        result.operator_id = id;
        result.operator_uid = uid;
        return result;
}

//--------------------------------
//
//--------------------------------
std::optional<OperatorDAO> OperatorRepository::update(int64_t id, const OperatorDAO& op) {
    sqlite3* h = db_->handle();
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

    sqlite3* h = db_->handle();
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
    sqlite3* h = db_->handle();
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
    sqlite3* h = db_->handle();
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
    db_->commit();
}