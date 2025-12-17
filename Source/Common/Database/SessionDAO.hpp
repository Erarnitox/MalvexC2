#pragma once

#include "Types.hpp"

//--------------------------------
//
//--------------------------------
struct SessionDAO {
    int64_t session_id{0};
    UUID session_uid;
    int port{0};
};