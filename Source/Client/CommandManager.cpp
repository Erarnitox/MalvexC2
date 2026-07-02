#include "CommandManager.hpp"

#include "Types.hpp"

CommandManager& CommandManager::instance() {
    static CommandManager instance;
    return instance;
}

void CommandManager::track(const UUID& uid, const UUID& victim_uid, std::string command_text) {
    std::lock_guard lock(mtx_);
    PendingCommand pending;
    pending.uid = uid;
    pending.victim_uid = victim_uid;
    pending.command_text = std::move(command_text);
    pending.kind = exfil::kind_from_command(pending.command_text);
    pending.posted_at = get_unix_time();
    pending.state = PendingCommand::State::Posted;
    commands_[uid] = std::move(pending);
}

void CommandManager::mark_completed(const UUID& uid) {
    std::lock_guard lock(mtx_);
    if (auto it = commands_.find(uid); it != commands_.end()) {
        it->second.state = PendingCommand::State::Completed;
    }
}

void CommandManager::mark_failed(const UUID& uid) {
    std::lock_guard lock(mtx_);
    if (auto it = commands_.find(uid); it != commands_.end()) {
        it->second.state = PendingCommand::State::Failed;
    }
}

void CommandManager::poll_timeouts(int64_t now, int64_t timeout_seconds) {
    std::lock_guard lock(mtx_);
    for (auto& [uid, pending] : commands_) {
        if (pending.state == PendingCommand::State::Posted &&
            now - pending.posted_at > timeout_seconds) {
            pending.state = PendingCommand::State::TimedOut;
        }
    }
}

std::vector<PendingCommand> CommandManager::pending() const {
    std::lock_guard lock(mtx_);
    std::vector<PendingCommand> out;
    out.reserve(commands_.size());
    for (const auto& [uid, cmd] : commands_) {
        out.push_back(cmd);
    }
    return out;
}

std::optional<PendingCommand> CommandManager::get(const UUID& uid) const {
    std::lock_guard lock(mtx_);
    if (auto it = commands_.find(uid); it != commands_.end()) {
        return it->second;
    }
    return std::nullopt;
}
