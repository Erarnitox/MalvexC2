#pragma once

#include <Remote.hpp>

#include <optional>
#include <string>

namespace session_handshake {

inline constexpr const char* operator_role = "OPERATOR";
inline constexpr const char* implant_role = "IMPLANT";
inline constexpr const char* bridge_waiting = "WAITING";
inline constexpr const char* bridge_ready = "READY";

inline std::string trim_line(std::string value) {
    const std::string whitespace = " \t\n\r\f\v";

    const auto start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

struct Credentials {
    std::string role;
    std::string username;
    std::string password;
};

inline void send(
    cpppwn::Remote& conn,
    const char* role,
    const std::string& username,
    const std::string& password) {
    conn.sendline(role);
    conn.sendline(username);
    conn.sendline(password);
}

inline std::optional<Credentials> receive(cpppwn::Remote& conn) {
    Credentials credentials{
        .role = trim_line(conn.recvline()),
        .username = trim_line(conn.recvline()),
        .password = trim_line(conn.recvline()),
    };

    if (credentials.role.empty() || credentials.username.empty() || credentials.password.empty()) {
        return std::nullopt;
    }

    return credentials;
}

} // namespace session_handshake
