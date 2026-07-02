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

    std::vector<ResultDAO> list() const override;
    std::optional<ResultDAO> get(int64_t id) const override;
    std::optional<ResultDAO> get(const UUID& uid) const override;
    ResultDAO create(const ResultDAO& op) override;
    std::optional<ResultDAO> update(int64_t id, const ResultDAO& op) override;
    bool remove(int64_t id) override;

    std::optional<ResultDAO> get_by_command_uid(const UUID& command_uid) const;
    std::vector<ResultDAO> list_for_victim(const UUID& victim_uid) const;
    std::vector<ResultDAO> list_chunks_for_command(const UUID& command_uid) const;
    ResultDAO upsert_chunk(const ResultDAO& chunk);
    bool remove_chunks_for_command(const UUID& command_uid);

    void commit();

private:
    void ensure_table();
    static ResultDAO row_to_dao(sqlite3_stmt* stmt);
};
