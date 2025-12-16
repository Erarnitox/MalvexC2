#include "VictimManager.hpp"

//-------------------------------------------------
//
//-------------------------------------------------
VictimManager& VictimManager::instance() {
    static VictimManager instance;
    return instance;
}