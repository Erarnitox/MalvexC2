#pragma once

class CommandManager {
public:
    // Delete copy and move constructors/assignments (singleton pattern)
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;
    CommandManager(CommandManager&&) = delete;
    CommandManager& operator=(CommandManager&&) = delete;
    explicit CommandManager() = default;

    static CommandManager& instance();

};