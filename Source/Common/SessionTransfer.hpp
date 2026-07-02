#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace session_transfer {

inline constexpr const char* kDownloadPrefix = "__malvex:download:";
inline constexpr const char* kUploadPrefix = "__malvex:upload:";
inline constexpr std::size_t kMaxTransferBytes = 16 * 1024 * 1024;

[[nodiscard]] inline bool is_valid_filename(std::string_view name) noexcept {
    if (name.empty() || name.size() > 255) {
        return false;
    }

    if (name.contains("..") || name.contains('/') || name.contains('\\') || name.contains('\0')) {
        return false;
    }

    return true;
}

[[nodiscard]] inline bool is_download_command(std::string_view cmd) noexcept {
    return cmd.starts_with(kDownloadPrefix);
}

[[nodiscard]] inline bool is_upload_command(std::string_view cmd) noexcept {
    return cmd.starts_with(kUploadPrefix);
}

[[nodiscard]] inline std::string build_download_command(std::string_view filename) {
    return std::string(kDownloadPrefix) + std::string(filename);
}

[[nodiscard]] inline std::string build_upload_command(std::string_view filename, std::size_t size) {
    return std::string(kUploadPrefix) + std::string(filename) + ":" + std::to_string(size);
}

[[nodiscard]] inline std::optional<std::string> parse_download_filename(std::string_view cmd) {
    if (!cmd.starts_with(kDownloadPrefix)) {
        return std::nullopt;
    }

    const auto filename = cmd.substr(std::string_view(kDownloadPrefix).size());
    if (!is_valid_filename(filename)) {
        return std::nullopt;
    }

    return std::string(filename);
}

[[nodiscard]] inline std::optional<std::pair<std::string, std::size_t>> parse_upload_command(std::string_view cmd) {
    if (!cmd.starts_with(kUploadPrefix)) {
        return std::nullopt;
    }

    const auto payload = cmd.substr(std::string_view(kUploadPrefix).size());
    const auto colon = payload.rfind(':');
    if (colon == std::string_view::npos || colon == 0) {
        return std::nullopt;
    }

    const auto filename = payload.substr(0, colon);
    if (!is_valid_filename(filename)) {
        return std::nullopt;
    }

    try {
        const auto size = std::stoull(std::string(payload.substr(colon + 1)));
        if (size == 0 || size > kMaxTransferBytes) {
            return std::nullopt;
        }
        return std::pair{std::string(filename), static_cast<std::size_t>(size)};
    } catch (...) {
        return std::nullopt;
    }
}

[[nodiscard]] inline bool is_error_payload(std::string_view payload) noexcept {
    return payload.starts_with("ERROR:");
}

} // namespace session_transfer
