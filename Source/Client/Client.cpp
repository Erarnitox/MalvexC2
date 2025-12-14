#include "Client.hpp"
#include "Config.hpp"

//-------------------------------------------------
//
//-------------------------------------------------
Client::Client(const std::string& db_path)
    : m_config( Config::instance(db_path)),
    m_rest_client("")
{

}

//-------------------------------------------------
//
//-------------------------------------------------
Client& Client::instance(const std::string& db_path) {
    static Client instance(db_path);
    return instance;
}