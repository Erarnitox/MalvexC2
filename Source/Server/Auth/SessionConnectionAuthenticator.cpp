#include "SessionConnectionAuthenticator.hpp"

//--------------------------------
//
//--------------------------------
SessionConnectionAuthenticator::SessionConnectionAuthenticator(
    IOperatorAuthenticator& operator_auth,
    IVictimTemplateAuthenticator& victim_auth)
    : operator_auth_(operator_auth),
      victim_auth_(victim_auth) {}

//--------------------------------
//
//--------------------------------
bool SessionConnectionAuthenticator::authenticate(
    const std::string& role,
    const std::string& username,
    const std::string& password) const {
    if (username.empty() || password.empty()) {
        return false;
    }

    if (role.starts_with(session_handshake::implant_role)) {
        return victim_auth_.authenticate(username, password);
    }

    if (role.starts_with(session_handshake::operator_role)) {
        return operator_auth_.authenticate(username, password);
    }

    return false;
}
