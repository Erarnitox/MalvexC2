#pragma once

#include <IRepository.hpp>
#include <Database.hpp>

#include "VictimDAO.hpp"

#include <string>
#include <memory>

class VictimRepository : public IRepository<VictimDAO> {
public:
    explicit VictimRepository(const std::string& db_path);

    ~VictimRepository() override = default;

    std::vector<VictimDAO> list() override;
    std::optional<VictimDAO> get(int64_t id) override;
    std::optional<VictimDAO> get(const UUID& uid) override;
    VictimDAO create(const VictimDAO& op) override;
    std::optional<VictimDAO> update(int64_t id, const VictimDAO& op) override;
    bool remove(int64_t id) override;

    std::optional<VictimDAO> update(const std::string& username, const VictimDAO& op);
    std::optional<VictimDAO> upsert(const std::string& username, const std::string& password);
    bool remove(const std::string& username);

    void commit();

private:
    void ensure_table();
    std::unique_ptr<Database> db_;
};