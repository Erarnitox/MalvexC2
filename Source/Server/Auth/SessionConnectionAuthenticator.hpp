#pragma once

#include "OperatorAuthenticator.hpp"
#include "VictimTemplateAuthenticator.hpp"

#include <SessionHandshake.hpp>

#include <string>

//--------------------------------
//
//--------------------------------
class SessionConnectionAuthenticator {
public:
    SessionConnectionAuthenticator(
        IOperatorAuthenticator& operator_auth,
        IVictimTemplateAuthenticator& victim_auth);

    [[nodiscard]] bool authenticate(
        const std::string& role,
        const std::string& username,
        const std::string& password) const;

private:
    IOperatorAuthenticator& operator_auth_;
    IVictimTemplateAuthenticator& victim_auth_;
};
