#pragma once

#include "Config.hpp"

#include <cpppwn.hpp>

#include <string>


class Client {
private:
    //LogManager& m_log_man;
    //CommandManager& m_cmd_man;
    //VictimManager& m_vic_man;
    //SessionManager& m_sess_man;
    Config& m_config;
    cpppwn::RESTClient m_rest_client;

public:
    // Delete copy and move constructors/assignments (singleton pattern)
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;
    Client(const std::string& db_path = "client.db");

    static Client& instance(const std::string& db_path = "client.db");
};