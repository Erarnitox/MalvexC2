#pragma once

#include <IRepository.hpp>
#include <Database.hpp>

#include "ResultDAO.hpp"

#include <string>
#include <memory>

class ResultRepository : public IRepository<ResultDAO> {
public:
    explicit ResultRepository(const std::string& db_path);

    ~ResultRepository() override = default;

    std::vector<ResultDAO> list() override;
    std::optional<ResultDAO> get(int64_t id) override;
    ResultDAO create(const ResultDAO& op) override;
    std::optional<ResultDAO> update(int64_t id, const ResultDAO& op) override;
    bool remove(int64_t id) override;

    void commit();

private:
    void ensure_table();
    std::unique_ptr<Database> db_;
};