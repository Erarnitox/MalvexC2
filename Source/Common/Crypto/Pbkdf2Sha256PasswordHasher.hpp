#pragma once

#include "IPasswordHasher.hpp"

#include <cstdint>
#include <string>
#include <string_view>

//--------------------------------
// PBKDF2-HMAC-SHA256 password hasher
//--------------------------------
class Pbkdf2Sha256PasswordHasher final : public IPasswordHasher {
public:
    static constexpr std::string_view algorithm_prefix = "$pbkdf2-sha256$";
    static constexpr int default_iterations = 600'000;
    static constexpr std::size_t salt_length = 16;
    static constexpr std::size_t hash_length = 32;

    [[nodiscard]] std::string hash(const std::string& plaintext) const override;
    [[nodiscard]] bool verify(const std::string& plaintext, const std::string& stored_hash) const override;
    [[nodiscard]] bool is_hashed(const std::string& value) const override;

private:
    [[nodiscard]] static std::string encode_base64(const unsigned char* data, std::size_t length);
    [[nodiscard]] static std::string decode_base64(std::string_view encoded);
    [[nodiscard]] static bool constant_time_equals(std::string_view left, std::string_view right);
};
