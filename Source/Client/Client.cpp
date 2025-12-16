#include "Client.hpp"
#include "Builder.hpp"
#include "CommandManager.hpp"
#include "Config.hpp"
#include "HttpUtils.hpp"
#include "LogManager.hpp"
#include "RESTClient.hpp"
#include "SessionManager.hpp"
#include "VictimManager.hpp"
#include <print>
#include <stdexcept>

//-------------------------------------------------
//
//-------------------------------------------------
Client::Client(const std::string& db_path) :
    m_config( Config::instance(db_path)),
    m_builder( Builder::instance() ),
    m_log_man( LogManager::instance() ),
    m_cmd_man( CommandManager::instance() ),
    m_vic_man( VictimManager::instance() ),
    m_sess_man( SessionManager::instance() ),
    m_rest_client("")
{

}

//-------------------------------------------------
//
//-------------------------------------------------
Client& Client::instance(const std::string& db_path) {
    static Client inst(db_path);
    return inst;
}

//-------------------------------------------------
//
//-------------------------------------------------
bool Client::hasServerSession() const noexcept {
    return m_config.has(Key::client_server_url_key)
        && m_config.has(Key::client_username_key)
        && m_config.has(Key::client_password_key)
        && m_config.has(Key::client_bearer_token_key);
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Client::getUsername() const noexcept {
    return m_config.get<std::string>(Key::client_username_key, "username");
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Client::getPassword() const noexcept {
    return m_config.get<std::string>(Key::client_password_key, "P4$$w0rd!");
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Client::getServerUrl() const noexcept {
    return m_config.get<std::string>(Key::client_server_url_key, "https://erarnitox.de:3000/attacker");
}

//-------------------------------------------------
//
//-------------------------------------------------
void Client::setUsername(const std::string& username) noexcept {
    m_config.set(Key::client_username_key, username);
}

//-------------------------------------------------
//
//-------------------------------------------------
void Client::setPassword(const std::string& password) noexcept {
    m_config.set(Key::client_password_key, password);
}

//-------------------------------------------------
//
//-------------------------------------------------
void Client::setServerUrl(const std::string& server_url) noexcept {
    HttpConfig conf;
    conf.follow_redirects = true;
    conf.max_redirects = 3;
    conf.verbose = true;
    conf.verify_ssl = false;

    if (server_url.ends_with("/")) {
        m_rest_client = cpppwn::RESTClient(server_url.substr(server_url.size() - 2), conf);
    } else {
        m_rest_client = cpppwn::RESTClient(server_url, conf);
    }
    m_config.set(Key::client_server_url_key, server_url);
}

//-------------------------------------------------
//
//-------------------------------------------------
void Client::setOutputPath(const std::string& output_dir) noexcept {
    m_config.set(Key::client_output_dir_key, output_dir);
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string Client::getOutputPath() const noexcept {
    return m_config.get<std::string>(Key::client_output_dir_key, "./outputs/");
}

//-------------------------------------------------
//
//-------------------------------------------------
bool Client::login() {
    m_rest_client.set_auth_basic(getUsername(), getPassword());

    // make test request
    try{
        return m_rest_client.get<bool>("/auth");
    } catch(const std::runtime_error& err) {
        std::println("Login Failed: {}", err.what());
        return false;
    }
}