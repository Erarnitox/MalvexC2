#pragma once

#include <IRepository.hpp>
#include <Database.hpp>

#include "LogDAO.hpp"

#include <string>
#include <memory>

class LogRepository : public IRepository<LogDAO> {
public:
    explicit LogRepository(const std::string& db_path);

    ~LogRepository() override = default;

    std::vector<LogDAO> list() const override;
    std::optional<LogDAO> get(int64_t id) const override;
    std::optional<LogDAO> get(const UUID& id) const override;
    LogDAO create(const LogDAO& log) override;
    std::optional<LogDAO> update(int64_t id, const LogDAO& log) override;
    bool remove(int64_t id) override;

    void commit();
    std::vector<LogDAO> list_recent(int limit = 1000);

private:
    void ensure_table();
};