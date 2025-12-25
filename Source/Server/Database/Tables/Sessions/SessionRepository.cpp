#include "SessionRepository.hpp"
#include "SessionDAO.hpp"
#include "sqlite3.h"

#include <sstream>
#include <iostream>

//--------------------------------
//
//--------------------------------
SessionRepository::SessionRepository(const std::string& db_path)
    : db_(std::make_unique<Database>(db_path)) {
    ensure_table();
}

//--------------------------------
//
//--------------------------------
void SessionRepository::ensure_table() {
    db_->exec(R"(
        CREATE TABLE IF NOT EXISTS sessions (
            session_id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_uid TEXT UNIQUE NOT NULL,
            port INTEGER NOT NULL
        );
    )");
}

//--------------------------------
//
//--------------------------------
std::vector<SessionDAO> SessionRepository::list() const {
    std::vector<SessionDAO> results;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "SELECT session_id, session_uid, port FROM sessions;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed: " + std::string(sqlite3_errmsg(h)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SessionDAO session;

        session.id = sqlite3_column_int64(stmt, 0);

        const char* uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        session.uid = uid_ptr ? uid_ptr : "";

        session.port = sqlite3_column_int(stmt, 2);

        results.push_back(std::move(session));
    }

    sqlite3_finalize(stmt);
    return results;
}

//--------------------------------
//
//--------------------------------
std::vector<SessionDAO> SessionRepository::list_by_port(int port) {
    std::vector<SessionDAO> results;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "SELECT session_id, session_uid, port FROM sessions WHERE port = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed: " + std::string(sqlite3_errmsg(h)));
    }

    sqlite3_bind_int(stmt, 1, port);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SessionDAO session;
        session.id = sqlite3_column_int64(stmt, 0);
        session.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        session.port = sqlite3_column_int(stmt, 2);

        results.push_back(std::move(session));
    }

    sqlite3_finalize(stmt);
    return results;
}

//--------------------------------
//
//--------------------------------
std::optional<SessionDAO> SessionRepository::get(int64_t id) const {
    std::optional<SessionDAO> opt;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT session_id, session_uid, port FROM sessions WHERE session_id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        SessionDAO session;
        session.id = sqlite3_column_int64(stmt, 0);
        session.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        session.port = sqlite3_column_int(stmt, 2);
        opt = session;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
std::optional<SessionDAO> SessionRepository::get(const UUID& uid) const {
    std::optional<SessionDAO> opt;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT session_id, session_uid, port FROM sessions WHERE session_id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        SessionDAO session;
        session.id = sqlite3_column_int64(stmt, 0);
        session.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        session.port = sqlite3_column_int(stmt, 2);
        opt = session;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
SessionDAO SessionRepository::create(const SessionDAO& session) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO sessions (session_uid, port) VALUES (?, ?);";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    UUID uid = session.uid.empty() ? generate_uuid() : session.uid;

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, session.port);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw SqliteException("insert failed");
    }

    sqlite3_finalize(stmt);
    int64_t id = sqlite3_last_insert_rowid(h);

    SessionDAO result = session;
    result.id = id;
    result.uid = uid;
    return result;
}

//--------------------------------
//
//--------------------------------
std::optional<SessionDAO> SessionRepository::update(int64_t id, const SessionDAO& session) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE sessions SET port = ? WHERE session_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int(stmt, 1, session.port);
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
bool SessionRepository::remove(int64_t id) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM sessions WHERE session_id = ?;";

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
bool SessionRepository::remove_by_port(int port) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM sessions WHERE port = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int(stmt, 1, port);

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
int SessionRepository::count() {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COUNT(*) FROM sessions;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

//--------------------------------
//
//--------------------------------
void SessionRepository::commit() {
    db_->commit();
}