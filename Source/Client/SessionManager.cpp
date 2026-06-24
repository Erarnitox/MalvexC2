#include "SessionManager.hpp"
#include "Logger.hpp"
#include "SessionDAO.hpp"
#include "Types.hpp"
#include <SessionHandshake.hpp>
#include <memory>

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

//-------------------------------------------------
//
//-------------------------------------------------
void SessionManager::startSession(
    const std::string& host,
    int64_t port,
    const std::string& username,
    const std::string& password) {
    const UUID uuid = generate_uuid();

    m_connections.emplace_back(std::make_unique<SessionConnection>(
        host,
        static_cast<int>(port),
        uuid,
        username,
        password));

    SessionDAO session;
    session.port = port;
    session.uid = uuid;

    m_sessions.push_back(session);
    m_port_to_connection[port] = uuid;
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string SessionManager::execute(const SessionDAO& session, const std::string& cmd) {
    const auto& session_uuid = m_port_to_connection[session.port];
    logger::debug("Trying to send Command [{}] to Session [{}]", cmd, session_uuid);

    if (session_uuid.empty()) return "";

    const auto& session_conn = std::find_if(m_connections.begin(), m_connections.end(), [session_uuid](const std::unique_ptr<SessionConnection>& sess_ptr) -> bool {
        return session_uuid == sess_ptr->get_uuid();
    });

    if (session_conn == m_connections.end()) return "";

    logger::debug("Sending Command: [{}]", cmd);
    return session_conn->get()->execute_cmd(cmd);
}

//-------------------------------------------------
//
//-------------------------------------------------
void SessionManager::close(const UUID& session_id) {
    if (session_id.empty()) return;

    const auto& session_conn = std::find_if(m_connections.begin(), m_connections.end(), [session_id](const std::unique_ptr<SessionConnection>& sess_ptr) -> bool {
        return session_id == sess_ptr->get_uuid();
    });

    const auto& session = std::find_if(m_sessions.begin(), m_sessions.end(), [session_id](const SessionDAO& sess) -> bool {
        return session_id == sess.uid;
    });

    if (session_conn != m_connections.end()) {
        session_conn->get()->stop();
        m_connections.erase(session_conn);
    }

    if (session != m_sessions.end()) {
        m_port_to_connection.erase(session->port);
        m_sessions.erase(session);
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
SessionConnection::SessionConnection(
    std::string host,
    int port,
    std::string sid,
    std::string username,
    std::string password)
    : session_id(std::move(sid)) {
    int retries = 0;
    while (not conn && retries < 5) {
        try {
            logger::info("Attempting connection to {}:{} (Attempt {}/5)...", host, port, retries + 1);
            conn = std::make_unique<cpppwn::Remote>(host, port, true, false);
            logger::success("Session established!");
        } catch (const std::exception& e) {
            ++retries;
            logger::warn("Connection failed: {}. Retrying in 2 seconds...", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    if (not conn) {
        logger::error("Could not reach C2 server after 5 attempts. Exiting.");
    } else {
        session_handshake::send(*conn, session_handshake::operator_role, username, password);
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string SessionConnection::execute_cmd(const std::string& cmd) {
    if (conn && conn->is_alive()) {
        conn->sendline(cmd);
    }

    const auto size = std::atol(trim_string(conn->recvline()).c_str());

    if (size > 0) {
        return conn->recv(size);
    } else {
        return "<NO DATA>";
    }
}

//-------------------------------------------------
//
//-------------------------------------------------
std::string SessionConnection::get_uuid() const {
    return session_id;
}

//-------------------------------------------------
//
//-------------------------------------------------
void SessionConnection::stop() {
    running = false;
}