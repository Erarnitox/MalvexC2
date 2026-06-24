#include "PasswordCredentialService.hpp"
#include "Pbkdf2Sha256PasswordHasher.hpp"
#include <stdexcept>

//--------------------------------
//
//--------------------------------
PasswordCredentialService::PasswordCredentialService(std::shared_ptr<IPasswordHasher> hasher)
    : hasher_(std::move(hasher)) {
    if (!hasher_) {
        throw std::invalid_argument("Password hasher must not be null");
    }
}

//--------------------------------
//
//--------------------------------
PasswordCredentialService& PasswordCredentialService::instance() {
    static PasswordCredentialService service{std::make_shared<Pbkdf2Sha256PasswordHasher>()};
    return service;
}

//--------------------------------
//
//--------------------------------
std::string PasswordCredentialService::hash_for_storage(const std::string& plaintext) const {
    return hasher_->hash(plaintext);
}

//--------------------------------
//
//--------------------------------
std::string PasswordCredentialService::resolve_for_storage(
    const std::string& plaintext,
    const std::string& existing_hash) const {
    if (!plaintext.empty()) {
        return hasher_->hash(plaintext);
    }

    if (!existing_hash.empty()) {
        return existing_hash;
    }

    throw std::invalid_argument("Password is required");
}

//--------------------------------
//
//--------------------------------
bool PasswordCredentialService::verify(const std::string& plaintext, const std::string& stored_value) const {
    if (hasher_->is_hashed(stored_value)) {
        return hasher_->verify(plaintext, stored_value);
    }

    if (plaintext.size() != stored_value.size()) {
        return false;
    }

    unsigned char mismatch = 0;
    for (std::size_t i = 0; i < plaintext.size(); ++i) {
        mismatch |= static_cast<unsigned char>(plaintext[i] ^ stored_value[i]);
    }

    return mismatch == 0;
}
