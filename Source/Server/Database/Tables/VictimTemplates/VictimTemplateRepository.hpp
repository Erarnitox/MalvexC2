#pragma once

#include <IRepository.hpp>
#include <Database.hpp>

#include "VictimTemplateDAO.hpp"

#include <string>
#include <memory>

class VictimTemplateRepository : public IRepository<VictimTemplateDAO> {
public:
    explicit VictimTemplateRepository(const std::string& db_path);

    ~VictimTemplateRepository() override = default;

    std::vector<VictimTemplateDAO> list() const override;
    std::optional<VictimTemplateDAO> get(int64_t id) const override;
    std::optional<VictimTemplateDAO> get(const UUID& uid) const override;
    VictimTemplateDAO create(const VictimTemplateDAO& op) override;
    std::optional<VictimTemplateDAO> update(int64_t id, const VictimTemplateDAO& op) override;
    bool remove(int64_t id) override;

    std::optional<VictimTemplateDAO> get_username(const std::string& username);

    void commit();

private:
    void ensure_table();
};