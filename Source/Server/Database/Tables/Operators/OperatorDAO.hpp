#pragma once
#include "Types.hpp"

struct OperatorDAO {
    int64_t operator_id{0};
    UUID operator_uid;
    std::string username;
    std::string password;
    int clearance{0};
};