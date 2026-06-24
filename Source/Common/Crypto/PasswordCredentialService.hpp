#pragma once

#include "IPasswordHasher.hpp"

#include <memory>
#include <string>

//--------------------------------
// Coordinates password hashing and verification for persistence layers
//--------------------------------
class PasswordCredentialService {
public:
    explicit PasswordCredentialService(std::shared_ptr<IPasswordHasher> hasher);

    [[nodiscard]] static PasswordCredentialService& instance();

    [[nodiscard]] std::string hash_for_storage(const std::string& plaintext) const;
    [[nodiscard]] std::string resolve_for_storage(
        const std::string& plaintext,
        const std::string& existing_hash) const;
    [[nodiscard]] bool verify(
        const std::string& plaintext,
        const std::string& stored_value) const;

private:
    std::shared_ptr<IPasswordHasher> hasher_;
};
