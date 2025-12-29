#include "SessionManager.hpp"
#include "SessionDAO.hpp"

//-------------------------------------------------
//
//-------------------------------------------------
SessionManager& SessionManager::instance() {
    static SessionManager instance;
    return instance;
}

//-------------------------------------------------
//
//-------------------------------------------------
const std::vector<SessionDAO>& SessionManager::getSessions() const noexcept {
    return m_sessions;
}

//-------------------------------------------------
//
//-------------------------------------------------
void SessionManager::setList(std::vector<SessionDAO>&& session_list) noexcept {
    m_sessions = std::move(session_list);
}

//-------------------------------------------------
//
//-------------------------------------------------
const SessionDAO SessionManager::getSession(int64_t id) const noexcept {
    auto it = std::find_if(m_sessions.begin(), m_sessions.end(), [id](const SessionDAO& v) -> bool {
        return v.id == id;
    });

    if (it != m_sessions.end()) {
        return *it;
    }

    return SessionDAO{};
}