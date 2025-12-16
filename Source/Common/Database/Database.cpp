#include "Database.hpp"

//--------------------------------
//
//--------------------------------
Database::Database(const std::string& path) {
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(db_) ? sqlite3_errmsg(db_) : "Could not open sqlite db";
        sqlite3_close(db_);
        db_ = nullptr;
        throw SqliteException(msg);
    }
    // use WAL for better concurrency
    exec("PRAGMA journal_mode=WAL;");
}

//--------------------------------
//
//--------------------------------
Database::~Database() {
    if (db_)
        sqlite3_close(db_);
}

//--------------------------------
//
//--------------------------------
void Database::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "sqlite exec error";
        sqlite3_free(err);
        throw SqliteException(msg);
    }
}

//--------------------------------
//
//--------------------------------
void Database::commit() {
    exec("COMMIT;");
}

//--------------------------------
//
//--------------------------------
void Database::query(const std::string& sql, const std::function<void(int, char**, char**)>& row_cb) {
    char* err = nullptr;
    auto callback = [](void* user, int cols, char** values, char** names) -> int {
        auto cb = reinterpret_cast<std::function<void(int, char**, char**)>*>(user);
        (*cb)(cols, values, names);
        return 0;
    };
    std::function<void(int, char**, char**)> cb = row_cb;
    int rc = sqlite3_exec(db_, sql.c_str(), callback, &cb, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "sqlite query error";
        sqlite3_free(err);
        throw SqliteException(msg);
    }
}
