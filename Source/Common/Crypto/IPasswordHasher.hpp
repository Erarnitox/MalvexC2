#pragma once

#include <string>

//--------------------------------
// Abstraction for password hashing
//--------------------------------
class IPasswordHasher {
public:
    virtual ~IPasswordHasher() = default;

    [[nodiscard]] virtual std::string hash(const std::string& plaintext) const = 0;
    [[nodiscard]] virtual bool verify(const std::string& plaintext, const std::string& stored_hash) const = 0;
    [[nodiscard]] virtual bool is_hashed(const std::string& value) const = 0;
};
