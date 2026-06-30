#pragma once

#include "OperatorAuthenticator.hpp"
#include "VictimTemplateAuthenticator.hpp"
#include "SessionConnectionAuthenticator.hpp"
#include "Services/SessionBridgeService.hpp"

struct ServerContext {
    IOperatorAuthenticator& operators;
    IVictimTemplateAuthenticator& victims;
    SessionConnectionAuthenticator& sessions;
    SessionBridgeService& session_bridge;
};
