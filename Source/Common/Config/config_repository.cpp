#include "config_repository.hpp"
#include "config_dao.hpp"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>

ConfigRepository::ConfigRepository(const std::string& db_path)
    : db_(std::make_unique<Database>(db_path)) {
    ensure_table();
}

void ConfigRepository::ensure_table() {
    db_->exec(R"(
        CREATE TABLE IF NOT EXISTS notes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            body TEXT NOT NULL,
            modified_at TEXT NOT NULL
        );
    )");
}

std::vector<ConfigDAO> ConfigRepository::list() {
    std::vector<ConfigDAO> result;
    db_->query("SELECT id, title, body, modified_at FROM notes ORDER BY modified_at DESC;",
        [&](int cols, char** values, char** names){
            ConfigDAO n;
            n.id = values[0] ? std::stoll(values[0]) : 0;
            n.title = values[1] ? values[1] : "";
            n.body = values[2] ? values[2] : "";
            n.modified_at = timepoint_from_iso(values[3] ? values[3] : "");
            result.push_back(std::move(n));
        });
    return result;
}

std::optional<ConfigDAO> ConfigRepository::get(int64_t id) {
    std::optional<ConfigDAO> opt;
    std::ostringstream sql;
    sql << "SELECT id, title, body, modified_at FROM notes WHERE id = " << id << " LIMIT 1;";
    db_->query(sql.str(), [&](int cols, char** values, char** names){
        if (cols >= 4) {
            ConfigDAO n;
            n.id = values[0] ? std::stoll(values[0]) : 0;
            n.title = values[1] ? values[1] : "";
            n.body = values[2] ? values[2] : "";
            n.modified_at = timepoint_from_iso(values[3] ? values[3] : "");
            opt = n;
        }
    });
    return opt;
}

ConfigDAO ConfigRepository::create(const ConfigDAO& note) {
    std::string t = timepoint_to_iso(note.modified_at);
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO notes (title, body, modified_at) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }
    sqlite3_bind_text(stmt, 1, note.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, note.body.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, t.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw SqliteException("insert step failed");
    }
    sqlite3_finalize(stmt);
    int64_t id = sqlite3_last_insert_rowid(h);
    Note copy = note;
    copy.id = id;
    return copy;
}

std::optional<Note> SqliteNoteRepository::update(int64_t id, const Note& note) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE notes SET title = ?, body = ?, modified_at = ? WHERE id = ?;";
    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }
    sqlite3_bind_text(stmt, 1, note.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, note.body.c_str(), -1, SQLITE_TRANSIENT);
    std::string t = timepoint_to_iso(note.modified_at);
    sqlite3_bind_text(stmt, 3, t.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
    sqlite3_finalize(stmt);
    return get(id);
}

bool SqliteNoteRepository::remove(int64_t id) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM notes WHERE id = ?;";
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
