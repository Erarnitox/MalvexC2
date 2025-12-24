#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct OperatorDAO : public IDao<OperatorDAO> {
    int64_t id{0};
    UUID uid;
    std::string username;
    std::string password;
    int clearance{0};
};

//--------------------------------
//
//--------------------------------
template <>
struct glz::meta<OperatorDAO> {
    using T = OperatorDAO;
    static constexpr auto value = object(
        "operator_id", &T::id,
        "operator_uid", &T::uid,
        "username", &T::username,
        "password", &T::password,
        "clearance", &T::clearance
    );
};