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

inline constexpr TimePoint kVictimOnlineThresholdSeconds = 300;

inline int compute_victim_status(TimePoint last_update, TimePoint now = static_cast<TimePoint>(get_unix_time())) {
    if (last_update == 0) {
        return 0;
    }

    return (now - last_update) <= kVictimOnlineThresholdSeconds ? 1 : 0;
}

inline void refresh_victim_status(VictimDAO& victim) {
    victim.status = compute_victim_status(victim.last_update);
}

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