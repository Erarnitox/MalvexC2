#pragma once

#include "Types.hpp"

//--------------------------------
//
//--------------------------------
struct LogDAO {
    int64_t log_id{0};
    UUID log_uid;
    std::string key;
    std::string value;
    TimePoint time;
};