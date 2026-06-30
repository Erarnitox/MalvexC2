#pragma once

namespace malvex::client {
class SessionManager;
class LogManager;
class VictimManager;

using SessionBridge = SessionManager;
using UiLogBuffer = LogManager;
using VictimCache = VictimManager;
} // namespace malvex::client
