#pragma once

#include "Types.hpp"
#include "IDao.hpp"

#include <string>
#include <vector>

//--------------------------------
//
//--------------------------------
struct OperatorDAO : public IDao<OperatorDAO> {
    int64_t id{0};
    UUID uid;
    std::string username;
    std::string password;       // write-only plaintext from API/setup
    std::string password_hash;  // hashed value persisted in database
    int clearance{0};

    [[nodiscard]] std::string to_public_json() const;
};

struct OperatorCreateRequest {
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
        "clearance", &T::clearance
    );
};

template <>
struct glz::meta<OperatorCreateRequest> {
    using T = OperatorCreateRequest;
    static constexpr auto value = object(
        "username", &T::username,
        "password", &T::password,
        "clearance", &T::clearance
    );
};

[[nodiscard]] std::string to_public_json_array(const std::vector<OperatorDAO>& items);
