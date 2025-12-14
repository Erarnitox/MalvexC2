#pragma once
#include "Types.hpp"

struct CommandDAO {
    int64_t command_id{0};
    UUID command_uid;
    int64_t prev{0};
    int64_t nonce{0};
    std::string command;
    std::string signature;
    int status{0};
};