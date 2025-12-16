#include "SessionManager.hpp"

//-------------------------------------------------
//
//-------------------------------------------------
SessionManager& SessionManager::instance() {
    static SessionManager instance;
    return instance;
}