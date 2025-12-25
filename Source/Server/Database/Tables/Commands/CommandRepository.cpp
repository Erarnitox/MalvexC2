#include "CommandRepository.hpp"
#include "CommandDAO.hpp"

//--------------------------------
//
//--------------------------------
CommandRepository::CommandRepository(const std::string& db_path) {
    set_db_path(db_path);
    ensure_table();
}

//--------------------------------
//
//--------------------------------
void CommandRepository::ensure_table() {
    auto db = open_readwrite();

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS commands (
            command_id INTEGER PRIMARY KEY AUTOINCREMENT,
            command_uid TEXT UNIQUE NOT NULL,
            client TEXT NOT NULL,
            prev INTEGER,
            nonce INTEGER,
            command TEXT NOT NULL,
            signature TEXT,
            status INTEGER
        );
    )");
}

//--------------------------------
//
//--------------------------------
std::vector<CommandDAO> CommandRepository::list() const {
    auto db = open_readonly();

    std::vector<CommandDAO> results;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "SELECT command_id, command_uid, client, prev, nonce, command, signature, status "
                      "FROM commands ORDER BY command_id DESC;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed: " + std::string(sqlite3_errmsg(h)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CommandDAO cmd;

        cmd.id = sqlite3_column_int64(stmt, 0);

        const char* uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        cmd.uid = uid_ptr ? uid_ptr : "";

        const char* client_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        cmd.client = client_ptr ? client_ptr : "";

        cmd.prev = sqlite3_column_int64(stmt, 3);
        cmd.nonce = sqlite3_column_int64(stmt, 4);

        const char* command_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        cmd.command = command_ptr ? command_ptr : "";

        const char* sig_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        cmd.signature = sig_ptr ? sig_ptr : "";

        cmd.status = sqlite3_column_int(stmt, 7);

        results.push_back(std::move(cmd));
    }

    sqlite3_finalize(stmt);
    return results;
}

//--------------------------------
//
//--------------------------------
std::optional<CommandDAO> CommandRepository::get(int64_t id) const {
    auto db = open_readonly();

    std::optional<CommandDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT command_id, command_uid, client, prev, nonce, command, signature, status "
                      "FROM commands WHERE command_id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        CommandDAO cmd;
        cmd.id = sqlite3_column_int64(stmt, 0);
        cmd.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        cmd.client = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        cmd.prev = sqlite3_column_int64(stmt, 3);
        cmd.nonce = sqlite3_column_int64(stmt, 4);
        cmd.command = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        cmd.signature = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        cmd.status = sqlite3_column_int(stmt, 7);
        opt = cmd;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
std::optional<CommandDAO> CommandRepository::get(const UUID& uid) const {
    auto db = open_readonly();

    std::optional<CommandDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT command_id, command_uid, client, prev, nonce, command, signature, status "
                      "FROM commands WHERE command_uid = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        CommandDAO cmd;
        cmd.id = sqlite3_column_int64(stmt, 0);
        cmd.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        cmd.client = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        cmd.prev = sqlite3_column_int64(stmt, 3);
        cmd.nonce = sqlite3_column_int64(stmt, 4);
        cmd.command = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        cmd.signature = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        cmd.status = sqlite3_column_int(stmt, 7);
        opt = cmd;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
CommandDAO CommandRepository::create(const CommandDAO& cmd) {
    auto db = open_readwrite();
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO commands (command_uid, client, prev, nonce, command, signature, status) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    UUID uid = cmd.uid.empty() ? generate_uuid() : cmd.uid;

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, cmd.client.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, cmd.prev);
    sqlite3_bind_int64(stmt, 4, cmd.nonce);
    sqlite3_bind_text(stmt, 5, cmd.command.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, cmd.signature.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, cmd.status);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw SqliteException("insert failed");
    }

    sqlite3_finalize(stmt);
    int64_t id = sqlite3_last_insert_rowid(h);

    CommandDAO result = cmd;
    result.id = id;
    result.uid = uid;
    return result;
}

//--------------------------------
//
//--------------------------------
std::optional<CommandDAO> CommandRepository::update(int64_t id, const CommandDAO& cmd) {
    auto db = open_readwrite();
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE commands SET prev = ?, nonce = ?, command = ?, "
                      "signature = ?, status = ? WHERE command_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, cmd.prev);
    sqlite3_bind_int64(stmt, 2, cmd.nonce);
    sqlite3_bind_text(stmt, 3, cmd.command.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, cmd.signature.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, cmd.status);
    sqlite3_bind_int64(stmt, 6, id);

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
bool CommandRepository::remove(int64_t id) {
    auto db = open_readwrite();
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM commands WHERE command_id = ?;";

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
void CommandRepository::commit() {
}

//--------------------------------
//
//--------------------------------
std::optional<CommandDAO> CommandRepository::get_for_client(const UUID& client_id) const {
    auto db = open_readwrite();

    std::optional<CommandDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT command_id, command_uid, client, prev, nonce, command, signature, status "
                      "FROM commands WHERE client = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, client_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        CommandDAO cmd;
        cmd.id = sqlite3_column_int64(stmt, 0);
        cmd.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        cmd.client = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        cmd.prev = sqlite3_column_int64(stmt, 3);
        cmd.nonce = sqlite3_column_int64(stmt, 4);
        cmd.command = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        cmd.signature = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        cmd.status = sqlite3_column_int(stmt, 7);
        opt = cmd;
    }

    sqlite3_finalize(stmt);
    return opt;
}
