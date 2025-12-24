#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct VictimDAO : public IDao<VictimDAO> {
    int64_t id{0};
    UUID uid;
    std::string internal_ip;
    std::string external_ip;
    std::string hostname;
    std::string username;
    std::string operating_system;
    TimePoint last_update;
    int status{0};
};

//--------------------------------
//
//--------------------------------
template <>
struct glz::meta<VictimDAO> {
    using T = VictimDAO;
    static constexpr auto value = object(
        "victim_id", &T::id,
        "victim_uid", &T::uid,
        "internal_ip", &T::internal_ip,
        "external_ip", &T::external_ip,
        "hostname", &T::hostname,
        "username", &T::username,
        "operating_system", &T::operating_system,
        "last_update", &T::last_update,
        "status", &T::status
    );
};