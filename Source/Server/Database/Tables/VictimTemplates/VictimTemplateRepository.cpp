#include "VictimTemplateRepository.hpp"
#include "VictimTemplateDAO.hpp"
#include "sqlite3.h"

#include <sstream>
#include <iostream>

//--------------------------------
//
//--------------------------------
VictimTemplateRepository::VictimTemplateRepository(const std::string& db_path)
    : db_(std::make_unique<Database>(db_path)) {
    ensure_table();
}

//--------------------------------
//
//--------------------------------
void VictimTemplateRepository::ensure_table() {
    db_->exec(R"(
        CREATE TABLE IF NOT EXISTS victim_templates (
            victim_template_id INTEGER PRIMARY KEY AUTOINCREMENT,
            victim_template_uid TEXT UNIQUE NOT NULL,
            username TEXT NOT NULL,
            password TEXT NOT NULL
        );
    )");
}

//--------------------------------
//
//--------------------------------
std::vector<VictimTemplateDAO> VictimTemplateRepository::list() {
    std::vector<VictimTemplateDAO> results;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "SELECT victim_template_id, victim_template_uid, username, password FROM victim_templates;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed: " + std::string(sqlite3_errmsg(h)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        VictimTemplateDAO op;

        op.id = sqlite3_column_int64(stmt, 0);

        const char* uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        op.uid = uid_ptr ? uid_ptr : "";

        const char* user_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        op.username = user_ptr ? user_ptr : "";

        const char* pass_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        op.password = pass_ptr ? pass_ptr : "";

        results.push_back(std::move(op));
    }

    sqlite3_finalize(stmt);
    return results;
}

//--------------------------------
//
//--------------------------------
std::optional<VictimTemplateDAO> VictimTemplateRepository::get(int64_t id) {
    std::optional<VictimTemplateDAO> opt;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT victim_template_id, victim_template_uid, username, password FROM victim_templates WHERE victim_template_id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        VictimTemplateDAO op;
        op.id = sqlite3_column_int64(stmt, 0);
        op.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        op.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        op.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        opt = op;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
std::optional<VictimTemplateDAO> VictimTemplateRepository::get(const UUID& uid) {
    std::optional<VictimTemplateDAO> opt;
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT victim_template_id, victim_template_uid, username, password FROM victim_templates WHERE victim_template_uid = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        VictimTemplateDAO op;
        op.id = sqlite3_column_int64(stmt, 0);
        op.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        op.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        op.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        opt = op;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
VictimTemplateDAO VictimTemplateRepository::create(const VictimTemplateDAO& op) {
    sqlite3* h = db_->handle();
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "INSERT INTO victim_templates (victim_template_uid, username, password) "
                         "VALUES (?, ?, ?);";

        if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw SqliteException("prepare failed");
        }

        UUID uid = op.uid.empty() ? generate_uuid() : op.uid;

        sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, op.username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, op.password.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw SqliteException("insert failed");
        }

        sqlite3_finalize(stmt);
        int64_t id = sqlite3_last_insert_rowid(h);

        VictimTemplateDAO result = op;
        result.id = id;
        result.uid = uid;
        return result;
}

//--------------------------------
//
//--------------------------------
std::optional<VictimTemplateDAO> VictimTemplateRepository::update(int64_t id, const VictimTemplateDAO& op) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE victim_templates SET username = ?, password = ? "
                        "WHERE victim_template_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, op.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, op.password.c_str(), -1, SQLITE_TRANSIENT);
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
bool VictimTemplateRepository::remove(int64_t id) {
    sqlite3* h = db_->handle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM victim_templates WHERE victim_template_id = ?;";

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
void VictimTemplateRepository::commit() {
    db_->commit();
}