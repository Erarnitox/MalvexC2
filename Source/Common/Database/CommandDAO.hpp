#pragma once

#include "Types.hpp"
#include "IDao.hpp"

//--------------------------------
//
//--------------------------------
struct CommandDAO : public IDao<CommandDAO> {
    int64_t id{0};
    UUID uid;
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
        "command_id", &T::id,
        "command_uid", &T::uid,
        "prev", &T::prev,
        "nonce", &T::nonce,
        "command", &T::command,
        "signature", &T::signature,
        "status", &T::status
    );
};