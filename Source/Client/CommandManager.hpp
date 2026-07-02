#pragma once

#include "ExfilEnvelope.hpp"
#include "Types.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct PendingCommand {
    UUID uid;
    UUID victim_uid;
    std::string command_text;
    exfil::ExfilKind kind{exfil::ExfilKind::Text};
    int64_t posted_at{0};
    enum class State { Posted, Completed, Failed, TimedOut } state{State::Posted};
};

class CommandManager {
public:
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;
    CommandManager(CommandManager&&) = delete;
    CommandManager& operator=(CommandManager&&) = delete;
    explicit CommandManager() = default;

    static CommandManager& instance();

    void track(const UUID& uid, const UUID& victim_uid, std::string command_text);
    void mark_completed(const UUID& uid);
    void mark_failed(const UUID& uid);
    void poll_timeouts(int64_t now, int64_t timeout_seconds = 600);
    [[nodiscard]] std::vector<PendingCommand> pending() const;
    [[nodiscard]] std::optional<PendingCommand> get(const UUID& uid) const;

private:
    mutable std::mutex mtx_;
    std::unordered_map<UUID, PendingCommand> commands_;
};
