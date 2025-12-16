#include "LogManager.hpp"

//-------------------------------------------------
//
//-------------------------------------------------
LogManager& LogManager::instance() {
    static LogManager instance;
    return instance;
}