#pragma once

#include "Types.hpp"
#include "IDao.hpp"

#include <string>
#include <vector>

//--------------------------------
//
//--------------------------------
struct VictimTemplateDAO : public IDao<VictimTemplateDAO> {
    int64_t id{0};
    UUID uid;
    std::string username;
    std::string password;       // write-only plaintext from API/setup
    std::string password_hash;  // hashed value persisted in database

    [[nodiscard]] std::string to_public_json() const;
};

struct VictimTemplateCreateRequest {
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
        "username", &T::username
    );
};

template <>
struct glz::meta<VictimTemplateCreateRequest> {
    using T = VictimTemplateCreateRequest;
    static constexpr auto value = object(
        "username", &T::username,
        "password", &T::password
    );
};

[[nodiscard]] std::string to_public_json_array(const std::vector<VictimTemplateDAO>& items);
