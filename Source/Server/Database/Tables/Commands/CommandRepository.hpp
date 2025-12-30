#pragma once

#include <IRepository.hpp>
#include <Database.hpp>

#include "CommandDAO.hpp"
#include "SQLiteCpp/Database.h"

#include <string>

class CommandRepository : public IRepository<CommandDAO> {
public:
    explicit CommandRepository(const std::string& db_path);

    ~CommandRepository() override = default;

    [[nodiscard]]
    std::vector<CommandDAO> list() const override;

    [[nodiscard]]
    std::optional<CommandDAO> get(int64_t id) const override;

    [[nodiscard]]
    std::optional<CommandDAO> get(const UUID& id) const override;

    CommandDAO create(const CommandDAO& comm) override;
    std::optional<CommandDAO> update(int64_t id, const CommandDAO& op) override;
    bool remove(int64_t id) override;

    [[nodiscard]]
    std::vector<CommandDAO> get_for_client(const UUID& client_id) const;
    void commit();

private:
    void ensure_table();
};