#pragma once

#include <sqlite3.h>
#include <string>
#include <stdexcept>
#include <functional>

//--------------------------------
//
//--------------------------------
class SqliteException : public std::runtime_error {
public:
    explicit SqliteException(std::string m) : std::runtime_error(std::move(m)) {}
};

//--------------------------------
//
//--------------------------------
class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    // Non-copyable
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void exec(const std::string& sql);
    void query(const std::string& sql, const std::function<void(int cols, char** values, char** names)>& row_cb);
    void commit();

    sqlite3* handle() noexcept { return db_; }

private:
    sqlite3* db_{nullptr};
};