#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct SessionDAO : public IDao<SessionDAO> {
    int64_t id{0};
    UUID uid;
    int port{0};
};

//--------------------------------
//
//--------------------------------
template <>
struct glz::meta<SessionDAO> {
    using T = SessionDAO;
    static constexpr auto value = object(
        "session_id", &T::id,
        "session_uid", &T::uid,
        "port", &T::port
    );
};