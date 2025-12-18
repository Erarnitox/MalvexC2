#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct CommandDAO : public IDao<CommandDAO> {
    int64_t command_id{0};
    UUID command_uid;
    int64_t prev{0};
    int64_t nonce{0};
    std::string command;
    std::string signature;
    int status{0};
};

//--------------------------------
//
//--------------------------------
template <>
struct glz::meta<CommandDAO> {
    using T = CommandDAO;
    static constexpr auto value = object(
        "command_id", &T::command_id,
        "command_uid", &T::command_uid,
        "prev", &T::prev,
        "nonce", &T::nonce,
        "command", &T::command,
        "signature", &T::signature,
        "status", &T::status
    );
};