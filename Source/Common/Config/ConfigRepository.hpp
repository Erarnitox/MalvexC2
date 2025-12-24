#pragma once

#include "ConfigDAO.hpp"

#include <IRepository.hpp>
#include <Database.hpp>

#include <string>
#include <memory>

class ConfigRepository : public IRepository<ConfigDAO> {
public:
    explicit ConfigRepository(const std::string& db_path);

    ~ConfigRepository() override = default;

    std::vector<ConfigDAO> list() override;
    std::optional<ConfigDAO> get(int64_t id) override;
    ConfigDAO create(const ConfigDAO& res) override;
    std::optional<ConfigDAO> update(int64_t id, const ConfigDAO& res) override;
    bool remove(int64_t id) override;

    std::optional<ConfigDAO> get(const std::string& key) override;
    std::optional<ConfigDAO> update(const std::string& key, const ConfigDAO& res);
    std::optional<ConfigDAO> upsert(const std::string& key, const std::string& value);
    bool remove(const std::string& key);

private:
    void ensure_table();
    std::unique_ptr<Database> db_;
};