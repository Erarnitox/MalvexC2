#pragma once

#include "SQLiteCpp/Database.h"
#include "Types.hpp"
#include <cstdint>
#include <vector>
#include <optional>

template <typename Res>
struct IRepository {
    virtual ~IRepository() = default;

    [[nodiscard]]
    virtual std::vector<Res> list() const = 0;

    [[nodiscard]]
    virtual std::optional<Res> get(int64_t id) const = 0;

    [[nodiscard]]
    virtual std::optional<Res> get(const UUID& uid) const = 0;

    virtual Res create(const Res& res) = 0;
    virtual std::optional<Res> update(int64_t id, const Res& res) = 0;
    virtual bool remove(int64_t id) = 0;

    void set_db_path(const std::string& db_path) {
        db_path_ = db_path;
    }

protected:
    std::string db_path_;

    void configure_db(SQLite::Database& db) {
        db.setBusyTimeout(3000);
        db.exec("PRAGMA journal_mode=WAL;");
        db.exec("PRAGMA synchronous=NORMAL;");
    }

    SQLite::Database open_readonly() const {
        SQLite::Database db(db_path_, SQLite::OPEN_READONLY);
        configure_db(db);
        return db;
    }

    SQLite::Database open_readwrite() const {
        SQLite::Database db(db_path_, SQLite::OPEN_READWRITE);
        configure_db(db);
        return db;
    }
};