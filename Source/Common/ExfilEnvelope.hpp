#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace exfil {

enum class ExfilKind {
    Text,
    Loot,
    Keylogger,
    Screenshot,
    File,
};

inline constexpr std::string_view kKindText = "text";
inline constexpr std::string_view kKindLoot = "loot";
inline constexpr std::string_view kKindKeylogger = "keylogger";
inline constexpr std::string_view kKindScreenshot = "screenshot";
inline constexpr std::string_view kKindFile = "file";

[[nodiscard]] inline std::string_view kind_to_string(ExfilKind kind) noexcept {
    switch (kind) {
    case ExfilKind::Loot: return kKindLoot;
    case ExfilKind::Keylogger: return kKindKeylogger;
    case ExfilKind::Screenshot: return kKindScreenshot;
    case ExfilKind::File: return kKindFile;
    case ExfilKind::Text:
    default: return kKindText;
    }
}

[[nodiscard]] inline ExfilKind kind_from_string(std::string_view kind) noexcept {
    if (kind == kKindLoot) return ExfilKind::Loot;
    if (kind == kKindKeylogger) return ExfilKind::Keylogger;
    if (kind == kKindScreenshot) return ExfilKind::Screenshot;
    if (kind == kKindFile) return ExfilKind::File;
    return ExfilKind::Text;
}

[[nodiscard]] inline ExfilKind kind_from_command(std::string_view command) noexcept {
    if (command.starts_with("loot")) return ExfilKind::Loot;
    if (command.starts_with("keylogger_start") || command.starts_with("keylogger_stop")) {
        return ExfilKind::Keylogger;
    }
    if (command.starts_with("screenshot")) return ExfilKind::Screenshot;
    if (command.starts_with("download ")) return ExfilKind::File;
    return ExfilKind::Text;
}

// Result status: 0 = in-progress/pending, 1 = success, 2 = failed
inline constexpr int kStatusPending = 0;
inline constexpr int kStatusSuccess = 1;
inline constexpr int kStatusFailed = 2;

inline constexpr std::size_t kMaxBeaconChunkBytes = 512 * 1024;
inline constexpr std::size_t kMaxBeaconExfilBytes = 4 * 1024 * 1024;
inline constexpr std::size_t kMaxFileBytes = 64 * 1024 * 1024;
inline constexpr std::size_t kMaxLootFiles = 500;
inline constexpr std::size_t kMaxLootTotalBytes = 32 * 1024 * 1024;
inline constexpr std::size_t kMaxLootFileBytes = 4 * 1024 * 1024;

} // namespace exfil
