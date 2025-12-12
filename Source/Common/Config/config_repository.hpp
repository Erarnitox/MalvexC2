#pragma once
#include <string>
#include "../i_repository.hpp"
#include "../database.hpp"
#include <memory>

#include "config_dao.hpp"

class ConfigRepository : public IRepository<ConfigDAO> {
public:
    explicit ConfigRepository(const std::string& db_path);

    ~ConfigRepository() override = default;

    std::vector<ConfigDAO> list() override;
    std::optional<ConfigDAO> get(int64_t id) override;
    ConfigDAO create(const ConfigDAO& res) override;
    std::optional<ConfigDAO> update(int64_t id, const ConfigDAO& res) override;
    bool remove(int64_t id) override;

    std::optional<ConfigDAO> get(const std::string& key);

private:
    void ensure_table();
    std::unique_ptr<Database> db_;
};