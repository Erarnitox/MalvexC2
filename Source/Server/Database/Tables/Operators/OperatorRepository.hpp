#pragma once

#include <IRepository.hpp>
#include <Database.hpp>

#include "OperatorDAO.hpp"

#include <string>
#include <memory>

class OperatorRepository : public IRepository<OperatorDAO> {
public:
    explicit OperatorRepository(const std::string& db_path);

    ~OperatorRepository() override = default;

    std::vector<OperatorDAO> list() const override;
    std::optional<OperatorDAO> get(int64_t id) const override;
    std::optional<OperatorDAO> get(const UUID& id) const override;
    OperatorDAO create(const OperatorDAO& op) override;
    std::optional<OperatorDAO> update(int64_t id, const OperatorDAO& op) override;
    bool remove(int64_t id) override;

    std::optional<OperatorDAO> get_username(const std::string& username);
    std::optional<OperatorDAO> update(const std::string& username, const OperatorDAO& op);
    std::optional<OperatorDAO> upsert(const std::string& username, const std::string& password);
    bool remove(const std::string& username);

    void commit();

private:
    void ensure_table();
};