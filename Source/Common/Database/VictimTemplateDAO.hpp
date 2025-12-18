#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct VictimTemplateDAO : public IDao {
    int64_t victim_template_id{0};
    UUID victim_template_uid;
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
        "victim_template_id", &T::victim_template_id,
        "victim_template_uid", &T::victim_template_uid,
        "username", &T::username,
        "password", &T::password
    );
};