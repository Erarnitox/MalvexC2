#include "VictimTemplateRepository.hpp"
#include "VictimTemplateDAO.hpp"
#include "PasswordCredentialService.hpp"
#include "Pbkdf2Sha256PasswordHasher.hpp"
#include "sqlite3.h"

#include <sstream>
#include <iostream>

namespace {

void map_password_column(VictimTemplateDAO& op, const char* stored_password) {
    op.password_hash = stored_password ? stored_password : "";
    op.password.clear();
}

[[nodiscard]] std::string resolve_password_hash(const VictimTemplateDAO& op) {
    auto& credentials = PasswordCredentialService::instance();

    if (!op.password.empty()) {
        return credentials.hash_for_storage(op.password);
    }

    if (!op.password_hash.empty()) {
        return op.password_hash;
    }

    throw std::invalid_argument("Victim template password is required");
}

void migrate_legacy_passwords(SQLite::Database& db) {
    sqlite3* handle = db.getHandle();
    sqlite3_stmt* select_stmt = nullptr;

    if (sqlite3_prepare_v2(
            handle,
            "SELECT victim_template_id, password FROM victim_templates;",
            -1,
            &select_stmt,
            nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed during victim template password migration");
    }

    Pbkdf2Sha256PasswordHasher hasher;
    auto& credentials = PasswordCredentialService::instance();

    while (sqlite3_step(select_stmt) == SQLITE_ROW) {
        const auto template_id = sqlite3_column_int64(select_stmt, 0);
        const char* stored_password = reinterpret_cast<const char*>(sqlite3_column_text(select_stmt, 1));

        if (stored_password == nullptr || hasher.is_hashed(stored_password)) {
            continue;
        }

        sqlite3_stmt* update_stmt = nullptr;
        if (sqlite3_prepare_v2(
                handle,
                "UPDATE victim_templates SET password = ? WHERE victim_template_id = ?;",
                -1,
                &update_stmt,
                nullptr) != SQLITE_OK) {
            sqlite3_finalize(select_stmt);
            throw SqliteException("prepare failed during victim template password migration update");
        }

        const auto hashed_password = credentials.hash_for_storage(stored_password);
        sqlite3_bind_text(update_stmt, 1, hashed_password.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(update_stmt, 2, template_id);

        if (sqlite3_step(update_stmt) != SQLITE_DONE) {
            sqlite3_finalize(update_stmt);
            sqlite3_finalize(select_stmt);
            throw SqliteException("update failed during victim template password migration");
        }

        sqlite3_finalize(update_stmt);
    }

    sqlite3_finalize(select_stmt);
}

} // namespace

//--------------------------------
//
//--------------------------------
VictimTemplateRepository::VictimTemplateRepository(const std::string& db_path) {
    set_db_path(db_path);
    ensure_table();
}

//--------------------------------
//
//--------------------------------
void VictimTemplateRepository::ensure_table() {
    auto db = open_readwrite();

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS victim_templates (
            victim_template_id INTEGER PRIMARY KEY AUTOINCREMENT,
            victim_template_uid TEXT UNIQUE NOT NULL,
            username TEXT NOT NULL,
            password TEXT NOT NULL
        );
    )");

    migrate_legacy_passwords(db);
}

//--------------------------------
//
//--------------------------------
std::vector<VictimTemplateDAO> VictimTemplateRepository::list() const {
    auto db = open_readonly();

    std::vector<VictimTemplateDAO> results;
    sqlite3* h = db.getHandle();
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
        map_password_column(op, pass_ptr);

        results.push_back(std::move(op));
    }

    sqlite3_finalize(stmt);
    return results;
}

//--------------------------------
//
//--------------------------------
std::optional<VictimTemplateDAO> VictimTemplateRepository::get(int64_t id) const {
    auto db = open_readonly();

    std::optional<VictimTemplateDAO> opt;
    sqlite3* h = db.getHandle();
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
        map_password_column(op, reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        opt = op;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
std::optional<VictimTemplateDAO> VictimTemplateRepository::get(const UUID& uid) const {
    auto db = open_readonly();

    std::optional<VictimTemplateDAO> opt;
    sqlite3* h = db.getHandle();
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
        map_password_column(op, reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        opt = op;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
std::optional<VictimTemplateDAO> VictimTemplateRepository::get_username(const std::string& username) {
    auto db = open_readonly();

    std::optional<VictimTemplateDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT victim_template_id, victim_template_uid, username, password FROM victim_templates WHERE username = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        VictimTemplateDAO op;
        op.id = sqlite3_column_int64(stmt, 0);
        op.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        op.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        map_password_column(op, reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        opt = op;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
VictimTemplateDAO VictimTemplateRepository::create(const VictimTemplateDAO& op) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO victim_templates (victim_template_uid, username, password) "
                        "VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    UUID uid = op.uid.empty() ? generate_uuid() : op.uid;
    const auto password_hash = resolve_password_hash(op);

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, op.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, password_hash.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw SqliteException("insert failed");
    }

    sqlite3_finalize(stmt);
    int64_t id = sqlite3_last_insert_rowid(h);

    VictimTemplateDAO result = op;
    result.id = id;
    result.uid = uid;
    result.password.clear();
    result.password_hash = password_hash;
    return result;
}

//--------------------------------
//
//--------------------------------
std::optional<VictimTemplateDAO> VictimTemplateRepository::update(int64_t id, const VictimTemplateDAO& op) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE victim_templates SET username = ?, password = ? "
                        "WHERE victim_template_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    std::string password_hash;
    if (!op.password.empty()) {
        password_hash = PasswordCredentialService::instance().hash_for_storage(op.password);
    } else if (!op.password_hash.empty()) {
        password_hash = op.password_hash;
    } else if (const auto existing = get(id)) {
        password_hash = existing->password_hash;
    } else {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, op.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_TRANSIENT);
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
bool VictimTemplateRepository::update_password_hash(int64_t id, const std::string& password_hash) {
    auto db = open_readwrite();
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE victim_templates SET password = ? WHERE victim_template_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, password_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);

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
bool VictimTemplateRepository::remove(int64_t id) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
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
}
