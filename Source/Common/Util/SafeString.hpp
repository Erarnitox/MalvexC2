#pragma once

#include <cstring>
#include <string>

namespace malvex {

[[nodiscard]] inline bool safe_copy(char* dest, std::size_t dest_size, std::string_view src) {
    if (dest == nullptr || dest_size == 0) {
        return false;
    }

    const std::size_t copy_len = std::min(dest_size - 1, src.size());
    if (copy_len > 0) {
        std::memcpy(dest, src.data(), copy_len);
    }
    dest[copy_len] = '\0';
    return true;
}

} // namespace malvex
