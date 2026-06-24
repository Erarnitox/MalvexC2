#include "Pbkdf2Sha256PasswordHasher.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <cctype>
#include <format>
#include <stdexcept>
#include <vector>

namespace {

//--------------------------------
//
//--------------------------------
[[nodiscard]] std::vector<unsigned char> derive_key(
    const std::string& password,
    const unsigned char* salt,
    std::size_t salt_len,
    int iterations) {
    std::vector<unsigned char> derived(Pbkdf2Sha256PasswordHasher::hash_length, 0);

    if (PKCS5_PBKDF2_HMAC(
            password.c_str(),
            static_cast<int>(password.size()),
            salt,
            static_cast<int>(salt_len),
            iterations,
            EVP_sha256(),
            static_cast<int>(derived.size()),
            derived.data()) != 1) {
        throw std::runtime_error("PBKDF2 key derivation failed");
    }

    return derived;
}

//--------------------------------
//
//--------------------------------
[[nodiscard]] std::array<unsigned char, Pbkdf2Sha256PasswordHasher::salt_length> random_salt() {
    std::array<unsigned char, Pbkdf2Sha256PasswordHasher::salt_length> salt{};
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
        throw std::runtime_error("Failed to generate cryptographic salt");
    }
    return salt;
}

} // namespace

//--------------------------------
//
//--------------------------------
std::string Pbkdf2Sha256PasswordHasher::encode_base64(const unsigned char* data, std::size_t length) {
    const int encoded_length = 4 * static_cast<int>((length + 2) / 3);
    std::string encoded(static_cast<std::size_t>(encoded_length), '\0');

    const int written = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(encoded.data()),
        data,
        static_cast<int>(length));

    if (written < 0) {
        throw std::runtime_error("Base64 encoding failed");
    }

    encoded.resize(static_cast<std::size_t>(written));
    return encoded;
}

//--------------------------------
//
//--------------------------------
std::string Pbkdf2Sha256PasswordHasher::decode_base64(std::string_view encoded) {
    std::string sanitized;
    sanitized.reserve(encoded.size());

    for (const char ch : encoded) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            sanitized.push_back(ch);
        }
    }

    std::vector<unsigned char> decoded((sanitized.size() * 3) / 4 + 1);
    const int written = EVP_DecodeBlock(
        decoded.data(),
        reinterpret_cast<const unsigned char*>(sanitized.data()),
        static_cast<int>(sanitized.size()));

    if (written < 0) {
        throw std::runtime_error("Base64 decoding failed");
    }

    std::size_t padding = 0;
    if (!sanitized.empty() && sanitized.back() == '=') {
        ++padding;
        if (sanitized.size() > 1 && sanitized[sanitized.size() - 2] == '=') {
            ++padding;
        }
    }

    const auto final_size = static_cast<std::size_t>(written) - padding;
    decoded.resize(final_size);
    return std::string(reinterpret_cast<const char*>(decoded.data()), decoded.size());
}

//--------------------------------
//
//--------------------------------
bool Pbkdf2Sha256PasswordHasher::constant_time_equals(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) {
        return false;
    }

    return CRYPTO_memcmp(left.data(), right.data(), left.size()) == 0;
}

//--------------------------------
//
//--------------------------------
std::string Pbkdf2Sha256PasswordHasher::hash(const std::string& plaintext) const {
    if (plaintext.empty()) {
        throw std::invalid_argument("Password must not be empty");
    }

    const auto salt = random_salt();
    const auto derived = derive_key(plaintext, salt.data(), salt.size(), default_iterations);

    return std::format(
        "{}{}${}${}",
        algorithm_prefix,
        default_iterations,
        encode_base64(salt.data(), salt.size()),
        encode_base64(derived.data(), derived.size()));
}

//--------------------------------
//
//--------------------------------
bool Pbkdf2Sha256PasswordHasher::verify(const std::string& plaintext, const std::string& stored_hash) const {
    if (plaintext.empty() || !is_hashed(stored_hash)) {
        return false;
    }

    const auto prefix_length = algorithm_prefix.size();
    const auto first_delimiter = stored_hash.find('$', prefix_length);
    const auto second_delimiter = stored_hash.find('$', first_delimiter + 1);

    if (first_delimiter == std::string::npos || second_delimiter == std::string::npos) {
        return false;
    }

    int iterations = 0;
    try {
        iterations = std::stoi(stored_hash.substr(prefix_length, first_delimiter - prefix_length));
    } catch (...) {
        return false;
    }

    if (iterations <= 0) {
        return false;
    }

    const auto salt_b64 = stored_hash.substr(first_delimiter + 1, second_delimiter - first_delimiter - 1);
    const auto hash_b64 = stored_hash.substr(second_delimiter + 1);

    std::string salt_bytes;
    std::string expected_hash_bytes;

    try {
        salt_bytes = decode_base64(salt_b64);
        expected_hash_bytes = decode_base64(hash_b64);
    } catch (...) {
        return false;
    }

    if (salt_bytes.empty() || expected_hash_bytes.empty()) {
        return false;
    }

    const auto derived = derive_key(
        plaintext,
        reinterpret_cast<const unsigned char*>(salt_bytes.data()),
        salt_bytes.size(),
        iterations);

    const std::string derived_string(reinterpret_cast<const char*>(derived.data()), derived.size());
    return constant_time_equals(derived_string, expected_hash_bytes);
}

//--------------------------------
//
//--------------------------------
bool Pbkdf2Sha256PasswordHasher::is_hashed(const std::string& value) const {
    return value.starts_with(algorithm_prefix);
}
