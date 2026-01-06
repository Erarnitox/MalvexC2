#pragma once

#include <iostream>
#include <format>
#include <string_view>
#include <source_location>
#include <stacktrace>
#include <chrono>

//-------------------------------------------------
//
//-------------------------------------------------
enum class LogLevel : int {
    DEBUG = 0,
    INFO = 1,
    SUCCESS = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 5,
    FATAL = 6,
    NONE = 7
};

//-------------------------------------------------
//
//-------------------------------------------------
constexpr inline std::string_view trim(std::string_view s) noexcept {
    constexpr std::string_view whitespace = " \t\n\r\f\v";
    const auto start = s.find_first_not_of(whitespace);
    if (start == std::string_view::npos) {
        return {};
    }

    s.remove_prefix(start);
    const auto end = s.find_last_not_of(whitespace);
    s.remove_suffix(s.size() - (end + 1));
    return s;
}

//-------------------------------------------------
//
//-------------------------------------------------
constexpr inline std::string trim_string(const std::string& s) noexcept {
    const std::string whitespace = " \t\n\r\f\v";

    size_t start = s.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }

    size_t end = s.find_last_not_of(whitespace);
    return s.substr(start, end - start + 1);
}

//-------------------------------------------------
//
//-------------------------------------------------
consteval LogLevel compiled_log_level() {
    #if defined(LOG_LEVEL_DEBUG)
        return LogLevel::DEBUG;
    #elif defined(LOG_LEVEL_WARN)
        return LogLevel::WARN;
     #elif defined(LOG_LEVEL_NONE)
        return LogLevel::NONE;
    #endif
    #ifdef NDEBUG
        return LogLevel::ERROR;
    #else
        return LogLevel::DEBUG;
    #endif
}

//-------------------------------------------------
//
//-------------------------------------------------
static constexpr LogLevel CURRENT_LOG_LEVEL = compiled_log_level();

//-------------------------------------------------
//
//-------------------------------------------------
namespace logger {
    //-------------------------------------------------
    //
    //-------------------------------------------------
    namespace color {
        constexpr std::string_view RESET   = "\033[0m";
        constexpr std::string_view RED     = "\033[31m";
        constexpr std::string_view GREEN   = "\033[32m";
        constexpr std::string_view YELLOW  = "\033[33m";
        constexpr std::string_view BLUE    = "\033[34m";
        constexpr std::string_view MAGENTA = "\033[35m";
        constexpr std::string_view CYAN    = "\033[36m";
        constexpr std::string_view BOLD    = "\033[1m";
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    inline std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        return std::format("{:%H:%M:%S}", now);
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    template<typename... Args>
    void log(LogLevel level, std::string_view prefix, std::string_view color, std::format_string<Args...> fmt, Args&&... args) {
        (void) level;

        // Define specific accent colors
        constexpr std::string_view BRACKET_COLOR = "\033[90m"; // Dark Gray
        constexpr std::string_view TIME_COLOR    = "\033[36m"; // Cyan
        constexpr std::string_view RESET         = "\033[0m";
        constexpr std::string_view BOLD          = "\033[1m";

        // 1. Format the user message first
        std::string message = std::format(fmt, std::forward<Args>(args)...);

        // 2. Assemble the highly colorful line
        // Pattern: [Time] [Prefix] Message
        std::cout << std::format(
            "{}[{}{}{}] [ {}{}{}{} ] {}{}{}\n",

            // Timestamp section: [ HH:MM:SS ]
            BRACKET_COLOR, TIME_COLOR, timestamp(), BRACKET_COLOR,

            // Prefix section: [ + ] or [ x ]
            color, BOLD, prefix, BRACKET_COLOR,

            // Message section (colored based on level)
            color, message, RESET
        );
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    template<typename... Args>
    constexpr void debug(std::format_string<Args...> fmt, Args&&... args) {
        if constexpr (CURRENT_LOG_LEVEL <= LogLevel::DEBUG) {
            log(LogLevel::DEBUG, "DEBUG", color::CYAN, fmt, std::forward<Args>(args)...);
        }
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    template<typename... Args>
    constexpr void info(std::format_string<Args...> fmt, Args&&... args) {
        if constexpr (CURRENT_LOG_LEVEL <= LogLevel::INFO) {
            log(LogLevel::INFO, "*", color::BLUE, fmt, std::forward<Args>(args)...);
        }
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    template<typename... Args>
    constexpr void success(std::format_string<Args...> fmt, Args&&... args) {
        if constexpr (CURRENT_LOG_LEVEL <= LogLevel::SUCCESS) {
            log(LogLevel::SUCCESS, "+", color::GREEN, fmt, std::forward<Args>(args)...);
        }
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    template<typename... Args>
    constexpr void warn(std::format_string<Args...> fmt, Args&&... args) {
        if constexpr (CURRENT_LOG_LEVEL <= LogLevel::WARN) {
            log(LogLevel::WARN, "!", color::YELLOW, fmt, std::forward<Args>(args)...);
        }
    }

    //-------------------------------------------------
    //
    //-------------------------------------------------
    template<typename... Args>
    constexpr void error(std::format_string<Args...> fmt, Args&&... args, const std::source_location loc = std::source_location::current()) {

        if constexpr (CURRENT_LOG_LEVEL <= LogLevel::ERROR) {
            // Log the main error message
            log(LogLevel::ERROR, "x", color::RED, fmt, std::forward<Args>(args)...);

            // Log Source Location
            std::cerr << std::format("    {}Location:{} {}:{} ({})\n",
                color::BOLD, color::RESET, loc.file_name(), loc.line(), loc.function_name());

            // Log Stacktrace (C++23)
            std::cerr << std::format("    {}Stacktrace:{}\n{}\n",
                color::BOLD, color::RESET, std::to_string(std::stacktrace::current()));
        }
    }
}