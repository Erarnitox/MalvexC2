#pragma once

#include "Types.hpp"

//--------------------------------
//
//--------------------------------
struct VictimDAO {
    int64_t victim_id{0};
    UUID victim_uid;
    std::string internal_ip;
    std::string external_ip;
    std::string hostname;
    std::string username;
    std::string operating_system;
    TimePoint last_update;
    int status{0};
};