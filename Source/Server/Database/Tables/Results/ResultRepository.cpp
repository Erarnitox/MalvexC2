#include "ResultRepository.hpp"
#include "ResultDAO.hpp"
#include "sqlite3.h"

namespace {

constexpr const char* kSelectColumns =
    "result_id, result_uid, command_uid, victim_uid, kind, status, "
    "chunk_index, chunk_total, created_at, data";

void try_alter_column(sqlite3* h, const char* sql) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

} // namespace

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
    try_alter_column(h, "ALTER TABLE results ADD COLUMN command_uid TEXT NOT NULL DEFAULT '';");
    try_alter_column(h, "ALTER TABLE results ADD COLUMN victim_uid TEXT NOT NULL DEFAULT '';");
    try_alter_column(h, "ALTER TABLE results ADD COLUMN kind TEXT NOT NULL DEFAULT 'text';");
    try_alter_column(h, "ALTER TABLE results ADD COLUMN status INTEGER NOT NULL DEFAULT 1;");
    try_alter_column(h, "ALTER TABLE results ADD COLUMN chunk_index INTEGER NOT NULL DEFAULT 0;");
    try_alter_column(h, "ALTER TABLE results ADD COLUMN chunk_total INTEGER NOT NULL DEFAULT 1;");
    try_alter_column(h, "ALTER TABLE results ADD COLUMN created_at INTEGER NOT NULL DEFAULT 0;");
}

ResultDAO ResultRepository::row_to_dao(sqlite3_stmt* stmt) {
    ResultDAO result;
    result.id = sqlite3_column_int64(stmt, 0);

    const char* uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    result.uid = uid_ptr ? uid_ptr : "";

    const char* command_uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    result.command_uid = command_uid_ptr ? command_uid_ptr : "";

    const char* victim_uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    result.victim_uid = victim_uid_ptr ? victim_uid_ptr : "";

    const char* kind_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    result.kind = kind_ptr ? kind_ptr : "text";

    result.status = sqlite3_column_int(stmt, 5);
    result.chunk_index = sqlite3_column_int(stmt, 6);
    result.chunk_total = sqlite3_column_int(stmt, 7);
    result.created_at = sqlite3_column_int64(stmt, 8);

    const char* data_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
    result.data = data_ptr ? data_ptr : "";

    return result;
}

std::vector<ResultDAO> ResultRepository::list() const {
    auto db = open_readonly();

    std::vector<ResultDAO> results;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;

    const std::string sql = std::string("SELECT ") + kSelectColumns + " FROM results WHERE chunk_total <= 1 OR chunk_index = -1 ORDER BY created_at DESC;";

    if (sqlite3_prepare_v2(h, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed: " + std::string(sqlite3_errmsg(h)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(row_to_dao(stmt));
    }

    sqlite3_finalize(stmt);
    return results;
}

std::optional<ResultDAO> ResultRepository::get(int64_t id) const {
    auto db = open_readonly();

    std::optional<ResultDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const std::string sql = std::string("SELECT ") + kSelectColumns + " FROM results WHERE result_id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        opt = row_to_dao(stmt);
    }

    sqlite3_finalize(stmt);
    return opt;
}

std::optional<ResultDAO> ResultRepository::get(const UUID& uid) const {
    auto db = open_readonly();

    std::optional<ResultDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const std::string sql = std::string("SELECT ") + kSelectColumns + " FROM results WHERE result_uid = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        opt = row_to_dao(stmt);
    }

    sqlite3_finalize(stmt);
    return opt;
}

std::optional<ResultDAO> ResultRepository::get_by_command_uid(const UUID& command_uid) const {
    auto db = open_readonly();

    std::optional<ResultDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const std::string sql = std::string("SELECT ") + kSelectColumns +
        " FROM results WHERE command_uid = ? AND chunk_index >= 0 ORDER BY chunk_index ASC LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, command_uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        opt = row_to_dao(stmt);
    }

    sqlite3_finalize(stmt);
    return opt;
}

std::vector<ResultDAO> ResultRepository::list_for_victim(const UUID& victim_uid) const {
    auto db = open_readonly();

    std::vector<ResultDAO> results;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const std::string sql = std::string("SELECT ") + kSelectColumns +
        " FROM results WHERE victim_uid = ? AND (chunk_total <= 1 OR chunk_index = -1) ORDER BY created_at DESC;";

    if (sqlite3_prepare_v2(h, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, victim_uid.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(row_to_dao(stmt));
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<ResultDAO> ResultRepository::list_chunks_for_command(const UUID& command_uid) const {
    auto db = open_readonly();

    std::vector<ResultDAO> results;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const std::string sql = std::string("SELECT ") + kSelectColumns +
        " FROM results WHERE command_uid = ? AND chunk_total > 1 AND chunk_index >= 0 ORDER BY chunk_index ASC;";

    if (sqlite3_prepare_v2(h, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, command_uid.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(row_to_dao(stmt));
    }

    sqlite3_finalize(stmt);
    return results;
}

ResultDAO ResultRepository::upsert_chunk(const ResultDAO& chunk) {
    auto db = open_readwrite();
    sqlite3* h = db.getHandle();

    sqlite3_stmt* find_stmt = nullptr;
    const char* find_sql =
        "SELECT result_id FROM results WHERE command_uid = ? AND chunk_index = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, find_sql, -1, &find_stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(find_stmt, 1, chunk.command_uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(find_stmt, 2, chunk.chunk_index);

    if (sqlite3_step(find_stmt) == SQLITE_ROW) {
        const int64_t existing_id = sqlite3_column_int64(find_stmt, 0);
        sqlite3_finalize(find_stmt);
        if (auto updated = update(existing_id, chunk)) {
            return *updated;
        }
        throw SqliteException("chunk update failed");
    }

    sqlite3_finalize(find_stmt);
    return create(chunk);
}

bool ResultRepository::remove_chunks_for_command(const UUID& command_uid) {
    auto db = open_readwrite();
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM results WHERE command_uid = ? AND chunk_total > 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, command_uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return sqlite3_changes(h) > 0;
}

ResultDAO ResultRepository::create(const ResultDAO& result) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO results (result_uid, command_uid, victim_uid, kind, status, "
        "chunk_index, chunk_total, created_at, data) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    UUID uid = result.uid.empty() ? generate_uuid() : result.uid;
    const int64_t created_at = result.created_at > 0 ? result.created_at : get_unix_time();

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, result.command_uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, result.victim_uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, result.kind.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, result.status);
    sqlite3_bind_int(stmt, 6, result.chunk_index);
    sqlite3_bind_int(stmt, 7, result.chunk_total);
    sqlite3_bind_int64(stmt, 8, created_at);
    sqlite3_bind_text(stmt, 9, result.data.c_str(), -1, SQLITE_TRANSIENT);

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
    created.created_at = created_at;
    return created;
}

std::optional<ResultDAO> ResultRepository::update(int64_t id, const ResultDAO& result) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "UPDATE results SET command_uid = ?, victim_uid = ?, kind = ?, status = ?, "
        "chunk_index = ?, chunk_total = ?, created_at = ?, data = ? WHERE result_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    const int64_t created_at = result.created_at > 0 ? result.created_at : get_unix_time();

    sqlite3_bind_text(stmt, 1, result.command_uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, result.victim_uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, result.kind.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, result.status);
    sqlite3_bind_int(stmt, 5, result.chunk_index);
    sqlite3_bind_int(stmt, 6, result.chunk_total);
    sqlite3_bind_int64(stmt, 7, created_at);
    sqlite3_bind_text(stmt, 8, result.data.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 9, id);

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
