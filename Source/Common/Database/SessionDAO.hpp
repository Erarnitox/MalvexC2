#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct SessionDAO : public IDao<SessionDAO> {
    int64_t session_id{0};
    UUID session_uid;
    int port{0};
};

//--------------------------------
//
//--------------------------------
template <>
struct glz::meta<SessionDAO> {
    using T = SessionDAO;
    static constexpr auto value = object(
        "session_id", &T::session_id,
        "session_uid", &T::session_uid,
        "port", &T::port
    );
};