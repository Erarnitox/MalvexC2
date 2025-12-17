#pragma once

#include "Types.hpp"

//--------------------------------
//
//--------------------------------
struct VictimTemplateDAO {
    int64_t victim_template_id{0};
    UUID victim_template_uid;
    std::string username;
    std::string password;
};