#pragma once

#include <IRepository.hpp>
#include <Database.hpp>

#include "CommandDAO.hpp"

#include <string>
#include <memory>

class CommandRepository : public IRepository<CommandDAO> {
public:
    explicit CommandRepository(const std::string& db_path);

    ~CommandRepository() override = default;

    std::vector<CommandDAO> list() override;
    std::optional<CommandDAO> get(int64_t id) override;
    CommandDAO create(const CommandDAO& comm) override;
    std::optional<CommandDAO> update(int64_t id, const CommandDAO& op) override;
    bool remove(int64_t id) override;

    void commit();

private:
    void ensure_table();
    std::unique_ptr<Database> db_;
};