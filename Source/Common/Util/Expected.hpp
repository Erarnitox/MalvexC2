#pragma once

#ifdef error
#undef error
#endif

#include <expected>
#include <string>

namespace malvex {

enum class ErrorCode {
    Network,
    Auth,
    Parse,
    NotFound,
    Timeout,
    Internal,
};

struct Error {
    ErrorCode code;
    std::string message;
};

template<typename T>
using Result = std::expected<T, Error>;

[[nodiscard]] inline Error make_error(ErrorCode code, std::string message) {
    return Error{code, std::move(message)};
}

} // namespace malvex
