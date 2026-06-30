#pragma once

#include <Logger.hpp>

#ifdef error
#undef error
#endif
#ifdef critical
#undef critical
#endif
#ifdef fatal
#undef fatal
#endif

namespace logger {

template<typename... Args>
constexpr void error(std::format_string<Args...> fmt, Args&&... args) {
    error_impl(std::stacktrace::current(), std::source_location::current(), fmt, std::forward<Args>(args)...);
}

template<typename... Args>
constexpr void critical(std::format_string<Args...> fmt, Args&&... args) {
    critical_impl(std::stacktrace::current(), std::source_location::current(), fmt, std::forward<Args>(args)...);
}

template<typename... Args>
constexpr void fatal(std::format_string<Args...> fmt, Args&&... args) {
    fatal_impl(std::stacktrace::current(), std::source_location::current(), fmt, std::forward<Args>(args)...);
}

} // namespace logger
