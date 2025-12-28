#pragma once

#include <string>
#include <vector>

#include "Database/CommandDAO.hpp"

struct CommandResult;

// Beacon request structure (from victim)
struct BeaconRequest {
    std::string victim_uid;
    std::string internal_ip;
    std::string external_ip;
    std::string hostname;
    std::string username;
    std::string operating_system;
    std::vector<CommandResult> command_results;  // Results from previous commands
};

// Command result structure
struct CommandResult {
    std::string command_uid;
    std::string result_data;
    int status;  // 0 = pending, 1 = success, 2 = failed
};

// Beacon response structure (to victim)
struct BeaconResponse {
    std::vector<CommandDAO> commands;  // Commands to execute
    int beacon_interval;               // Seconds until next beacon
    bool should_exit;                  // Kill switch
};