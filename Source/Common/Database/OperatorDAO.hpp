#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct OperatorDAO : public IDao {
    int64_t operator_id{0};
    UUID operator_uid;
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
        "operator_id", &T::operator_id,
        "operator_uid", &T::operator_uid,
        "username", &T::username,
        "password", &T::password,
        "clearance", &T::clearance
    );
};