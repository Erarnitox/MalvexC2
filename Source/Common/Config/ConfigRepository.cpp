#include "ConfigRepository.hpp"

#include <SQLiteCpp/Transaction.h>

namespace {

void configure_db(SQLite::Database& db) {
    db.setBusyTimeout(3000);
    db.exec("PRAGMA journal_mode=WAL;");
    db.exec("PRAGMA synchronous=NORMAL;");
}

}

//--------------------------------
//
//--------------------------------
ConfigRepository::ConfigRepository(std::string db_path)
    : db_path_(std::move(db_path)) {
    ensure_table();
}

//--------------------------------
//
//--------------------------------
SQLite::Database ConfigRepository::open_readonly() const {
    SQLite::Database db(db_path_, SQLite::OPEN_READONLY);
    configure_db(db);
    return db;
}

//--------------------------------
//
//--------------------------------
SQLite::Database ConfigRepository::open_readwrite() const {
    SQLite::Database db(db_path_, SQLite::OPEN_READWRITE);
    configure_db(db);
    return db;
}

//--------------------------------
//
//--------------------------------
void ConfigRepository::ensure_table() {
    auto db = open_readwrite();
    db.exec(R"(
        CREATE TABLE IF NOT EXISTS config (
            config_id INTEGER PRIMARY KEY AUTOINCREMENT,
            key TEXT NOT NULL UNIQUE,
            value TEXT NOT NULL
        );
    )");
}

//--------------------------------
//
//--------------------------------
std::vector<ConfigDAO> ConfigRepository::list() const {
    auto db = open_readonly();

    SQLite::Statement stmt(
        db,
        "SELECT config_id, key, value FROM config ORDER BY key;"
    );

    std::vector<ConfigDAO> result;
    while (stmt.executeStep()) {
        result.push_back({
            stmt.getColumn(0).getInt(),
            stmt.getColumn(1).getString(),
            stmt.getColumn(2).getString()
        });
    }
    return result;
}

//--------------------------------
//
//--------------------------------
std::optional<ConfigDAO> ConfigRepository::get(int64_t id) const {
    auto db = open_readonly();

    SQLite::Statement stmt(
        db,
        "SELECT config_id, key, value FROM config WHERE config_id = ? LIMIT 1;"
    );
    stmt.bind(1, id);

    if (!stmt.executeStep())
        return std::nullopt;

    return ConfigDAO{
        stmt.getColumn(0).getInt(),
        stmt.getColumn(1).getString(),
        stmt.getColumn(2).getString()
    };
}

//--------------------------------
//
//--------------------------------
std::optional<ConfigDAO> ConfigRepository::get(const std::string& key) const {
    auto db = open_readonly();

    SQLite::Statement stmt(
        db,
        "SELECT config_id, key, value FROM config WHERE key = ? LIMIT 1;"
    );
    stmt.bind(1, key);

    if (!stmt.executeStep())
        return std::nullopt;

    return ConfigDAO{
        stmt.getColumn(0).getInt(),
        stmt.getColumn(1).getString(),
        stmt.getColumn(2).getString()
    };
}

//--------------------------------
//
//--------------------------------
ConfigDAO ConfigRepository::create(const ConfigDAO& config) {
    auto db = open_readwrite();

    SQLite::Statement stmt(
        db,
        "INSERT INTO config (key, value) VALUES (?, ?);"
    );
    stmt.bind(1, config.key);
    stmt.bind(2, config.value);
    stmt.exec();

    ConfigDAO result = config;
    result.id = db.getLastInsertRowid();
    return result;
}

//--------------------------------
//
//--------------------------------
std::optional<ConfigDAO> ConfigRepository::update(
    int64_t id,
    const ConfigDAO& config
) {
    auto db = open_readwrite();

    SQLite::Statement stmt(
        db,
        "UPDATE config SET key = ?, value = ? WHERE config_id = ?;"
    );
    stmt.bind(1, config.key);
    stmt.bind(2, config.value);
    stmt.bind(3, id);

    if (stmt.exec() == 0)
        return std::nullopt;

    return get(id);
}

//--------------------------------
//
//--------------------------------
std::optional<ConfigDAO> ConfigRepository::upsert(
    const std::string& key,
    const std::string& value
) {
    auto db = open_readwrite();

    SQLite::Statement stmt(
        db,
        "INSERT INTO config (key, value) VALUES (?, ?) "
        "ON CONFLICT(key) DO UPDATE SET value = excluded.value;"
    );
    stmt.bind(1, key);
    stmt.bind(2, value);
    stmt.exec();

    return get(key);
}

//--------------------------------
//
//--------------------------------
bool ConfigRepository::remove(int64_t id) {
    auto db = open_readwrite();

    SQLite::Statement stmt(
        db,
        "DELETE FROM config WHERE config_id = ?;"
    );
    stmt.bind(1, id);
    stmt.exec();

    return db.getChanges() > 0;
}

//--------------------------------
//
//--------------------------------
bool ConfigRepository::remove(const std::string& key) {
    auto db = open_readwrite();

    SQLite::Statement stmt(
        db,
        "DELETE FROM config WHERE key = ?;"
    );
    stmt.bind(1, key);
    stmt.exec();

    return db.getChanges() > 0;
}
