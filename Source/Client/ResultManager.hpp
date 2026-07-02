#pragma once

#include "CommandManager.hpp"
#include "ResultDAO.hpp"
#include "ResultDecoders.hpp"
#include "RestGateway.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct LocalArtifact {
    ResultDAO result;
    std::string command_text;
    std::string source{"beacon"};
};

class ResultManager {
public:
    ResultManager(const ResultManager&) = delete;
    ResultManager& operator=(const ResultManager&) = delete;
    ResultManager(ResultManager&&) = delete;
    ResultManager& operator=(ResultManager&&) = delete;
    explicit ResultManager() = default;

    static ResultManager& instance();

    [[nodiscard]] bool refresh(RestGateway& gateway);
    [[nodiscard]] bool refresh_for_victim(RestGateway& gateway, const UUID& victim_uid);
    void poll_pending(RestGateway& gateway, CommandManager& commands);
    void add_local_artifact(ResultDAO result, std::string command_text, std::string source = "session");
    void track_command_text(const UUID& command_uid, const std::string& command_text);

    [[nodiscard]] std::vector<ResultDAO> results() const;
    [[nodiscard]] std::optional<ResultDAO> get(const UUID& command_uid) const;
    [[nodiscard]] DecodedArtifact decode(const UUID& command_uid, const std::string& command_text = "") const;
    [[nodiscard]] bool export_artifact(
        const UUID& command_uid,
        const std::string& path,
        const std::string& command_text = "") const;

private:
    mutable std::mutex mtx_;
    std::unordered_map<UUID, ResultDAO> cache_;
    std::unordered_map<UUID, std::string> command_texts_;
    std::unordered_map<UUID, LocalArtifact> local_only_;
    ResultDecoderRegistry decoders_;
};
