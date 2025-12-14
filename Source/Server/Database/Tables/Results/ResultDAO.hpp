#pragma once
#include "Types.hpp"

struct ResultDAO {
    int64_t result_id{0};
    UUID result_uid;
    std::string data;
};