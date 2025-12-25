#pragma once

#include "ConfigDAO.hpp"
#include "SQLiteCpp/Database.h"

#include <IRepository.hpp>
#include <Database.hpp>

#include <string>
#include <memory>

class ConfigRepository : IRepository<ConfigDAO>{
public:
    explicit ConfigRepository(std::string db_path);

    void ensure_table();

    [[nodiscard]]
    std::vector<ConfigDAO> list() const override;

    [[nodiscard]]
    std::optional<ConfigDAO> get(int64_t id) const override;

    [[nodiscard]]
    std::optional<ConfigDAO> get(const std::string& key) const override;

    ConfigDAO create(const ConfigDAO& config) override;

    std::optional<ConfigDAO> update(int64_t id, const ConfigDAO& config) override;

    std::optional<ConfigDAO> upsert(const std::string& key, const std::string& value);

    bool remove(int64_t id) override;
    bool remove(const std::string& key);

private:
    SQLite::Database open_readonly() const;
    SQLite::Database open_readwrite() const;

    std::string db_path_;
};