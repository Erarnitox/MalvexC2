#include "CommandManager.hpp"

//-------------------------------------------------
//
//-------------------------------------------------
CommandManager& CommandManager::instance() {
    static CommandManager instance;
    return instance;
}