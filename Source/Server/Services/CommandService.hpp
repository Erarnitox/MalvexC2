#pragma once

#include "CommandDAO.hpp"
#include "IManager.hpp"

#include <algorithm>
#include <vector>

class ICommandService {
public:
    virtual ~ICommandService() = default;
    virtual std::vector<CommandDAO> pending_for_client(const UUID& client_uid) = 0;
    virtual std::optional<CommandDAO> get_by_uid(const UUID& uid) = 0;
    virtual void mark_sent(CommandDAO& command) = 0;
    virtual void mark_completed(CommandDAO& command) = 0;
};

class CommandService final : public ICommandService {
public:
    explicit CommandService(Manager<CommandDAO, CommandRepository>& manager) : manager_(manager) {}

    std::vector<CommandDAO> pending_for_client(const UUID& client_uid) override {
        auto command_list = manager_.get_repo()->get_for_client(client_uid);
        std::erase_if(command_list, [](const CommandDAO& cmd) { return cmd.status > 0; });
        return command_list;
    }

    std::optional<CommandDAO> get_by_uid(const UUID& uid) override {
        return manager_.get_by_uid(uid);
    }

    void mark_sent(CommandDAO& command) override {
        command.status = 1;
        manager_.update(command.id, command);
    }

    void mark_completed(CommandDAO& command) override {
        command.status = 2;
        manager_.update(command.id, command);
    }

private:
    Manager<CommandDAO, CommandRepository>& manager_;
};
