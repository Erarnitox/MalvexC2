#pragma once
#include "BaseRepository.hpp"
#include "DAOs.hpp"
#include <sstream>
#include <iomanip>

class VictimRepository : public BaseRepository<VictimDAO> {
public:
    explicit VictimRepository(std::shared_ptr<Database> db)
        : BaseRepository(db) {
        ensure_table();
    }

    void ensure_table() override {
        db_->exec(R"(
            CREATE TABLE IF NOT EXISTS victims (
                victim_id INTEGER PRIMARY KEY AUTOINCREMENT,
                victim_uid TEXT UNIQUE NOT NULL,
                internal_ip TEXT,
                external_ip TEXT,
                hostname TEXT,
                username TEXT,
                operating_system TEXT,
                last_update TEXT,
                status INTEGER
            );
        )");
    }

    std::vector<VictimDAO> list() override {
        std::vector<VictimDAO> result;
        db_->query("SELECT victim_id, victim_uid, internal_ip, external_ip, hostname, "
                  "username, operating_system, last_update, status FROM victims "
                  "ORDER BY last_update DESC;",
            [&](int cols, char** values, char** names) {
                VictimDAO v;
                v.victim_id = values[0] ? std::stoll(values[0]) : 0;
                v.victim_uid = values[1] ? values[1] : "";
                v.internal_ip = values[2] ? values[2] : "";
                v.external_ip = values[3] ? values[3] : "";
                v.hostname = values[4] ? values[4] : "";
                v.username = values[5] ? values[5] : "";
                v.operating_system = values[6] ? values[6] : "";
                v.last_update = timepoint_from_iso(values[7] ? values[7] : "");
                v.status = values[8] ? std::stoi(values[8]) : 0;
                result.push_back(std::move(v));
            });
        return result;
    }

    std::vector<VictimDAO> list_by_status(int status) {
        std::vector<VictimDAO> result;
        sqlite3* h = db_->handle();
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT victim_id, victim_uid, internal_ip, external_ip, hostname, "
                         "username, operating_system, last_update, status FROM victims "
                         "WHERE status = ? ORDER BY last_update DESC;";

        if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw SqliteException("prepare failed");
        }

        sqlite3_bind_int(stmt, 1, status);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            VictimDAO v;
            v.victim_id = sqlite3_column_int64(stmt, 0);
            v.victim_uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            v.internal_ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            v.external_ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            v.hostname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            v.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            v.operating_system = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            v.last_update = timepoint_from_iso(
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
            v.status = sqlite3_column_int(stmt, 8);
            result.push_back(std::move(v));
        }

        sqlite3_finalize(stmt);
        return result;
    }

    std::optional<VictimDAO> get(int64_t id) override {
        std::optional<VictimDAO> opt;
        sqlite3* h = db_->handle();
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT victim_id, victim_uid, internal_ip, external_ip, hostname, "
                         "username, operating_system, last_update, status "
                         "FROM victims WHERE victim_id = ? LIMIT 1;";

        if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw SqliteException("prepare failed");
        }

        sqlite3_bind_int64(stmt, 1, id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            VictimDAO v;
            v.victim_id = sqlite3_column_int64(stmt, 0);
            v.victim_uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            v.internal_ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            v.external_ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            v.hostname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            v.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            v.operating_system = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            v.last_update = timepoint_from_iso(
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
            v.status = sqlite3_column_int(stmt, 8);
            opt = v;
        }

        sqlite3_finalize(stmt);
        return opt;
    }

    std::optional<VictimDAO> get_by_uid(const UUID& uid) override {
        std::optional<VictimDAO> opt;
        sqlite3* h = db_->handle();
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT victim_id, victim_uid, internal_ip, external_ip, hostname, "
                         "username, operating_system, last_update, status "
                         "FROM victims WHERE victim_uid = ? LIMIT 1;";

        if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw SqliteException("prepare failed");
        }

        sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            VictimDAO v;
            v.victim_id = sqlite3_column_int64(stmt, 0);
            v.victim_uid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            v.internal_ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            v.external_ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            v.hostname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            v.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            v.operating_system = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            v.last_update = timepoint_from_iso(
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
            v.status = sqlite3_column_int(stmt, 8);
            opt = v;
        }

        sqlite3_finalize(stmt);
        return opt;
    }

    VictimDAO create(const VictimDAO& victim) override {
        sqlite3* h = db_->handle();
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "INSERT INTO victims (victim_uid, internal_ip, external_ip, hostname, "
                         "username, operating_system, last_update, status) "
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

        if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw SqliteException("prepare failed");
        }

        UUID uid = victim.victim_uid.empty() ? generate_uuid() : victim.victim_uid;
        std::string timestamp = timepoint_to_iso(victim.last_update);

        sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, victim.internal_ip.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, victim.external_ip.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, victim.hostname.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, victim.username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, victim.operating_system.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 7, timestamp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 8, victim.status);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw SqliteException("insert failed");
        }

        sqlite3_finalize(stmt);
        int64_t id = sqlite3_last_insert_rowid(h);

        VictimDAO result = victim;
        result.victim_id = id;
        result.victim_uid = uid;
        return result;
    }

    std::optional<VictimDAO> update(int64_t id, const VictimDAO& victim) override {
        sqlite3* h = db_->handle();
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "UPDATE victims SET internal_ip = ?, external_ip = ?, hostname = ?, "
                         "username = ?, operating_system = ?, last_update = ?, status = ? "
                         "WHERE victim_id = ?;";

        if (sqlite3_prepare_v2(h, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw SqliteException("prepare failed");
        }

        std::string timestamp = timepoint_to_iso(victim.last_update);

        sqlite3_bind_text(stmt, 1, victim.internal_ip.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, victim.external_ip.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, victim.hostname.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, victim.username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, victim.operating_system.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, timestamp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 7, victim.status);
        sqlite3_bind_int64(stmt, 8, id);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return std::nullopt;
        }

        sqlite3_finalize(stmt);
        return get(id);
    }

    bool remove(int64_t id) override {
        sqlite3* h = db_->handle();
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

private:
    std::string timepoint_to_iso(const Timestamp& tp) {
        auto time = std::chrono::system_clock::to_time_t(tp);
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    }

    Timestamp timepoint_from_iso(const std::string& str) {
        if (str.empty()) return std::chrono::system_clock::now();
        std::tm tm = {};
        std::istringstream ss(str);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
};