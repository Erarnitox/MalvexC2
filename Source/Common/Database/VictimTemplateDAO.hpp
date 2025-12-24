#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct VictimTemplateDAO : public IDao<VictimTemplateDAO> {
    int64_t id{0};
    UUID uid;
    std::string username;
    std::string password;
};

//--------------------------------
//
//--------------------------------
template <>
struct glz::meta<VictimTemplateDAO> {
    using T = VictimTemplateDAO;
    static constexpr auto value = object(
        "victim_template_id", &T::id,
        "victim_template_uid", &T::uid,
        "username", &T::username,
        "password", &T::password
    );
};