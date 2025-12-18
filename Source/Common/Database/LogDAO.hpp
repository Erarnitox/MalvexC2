#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct LogDAO : public IDao<LogDAO> {
    int64_t log_id{0};
    UUID log_uid;
    std::string key;
    std::string value;
    TimePoint time;
};

//--------------------------------
//
//--------------------------------
template <>
struct glz::meta<LogDAO> {
    using T = LogDAO;
    static constexpr auto value = object(
        "log_id", &T::log_id,
        "log_uid", &T::log_uid,
        "key", &T::key,
        "value", &T::value,
        "time", &T::time
    );
};