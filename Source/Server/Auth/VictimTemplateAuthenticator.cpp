#include "VictimTemplateAuthenticator.hpp"

#include <Pbkdf2Sha256PasswordHasher.hpp>

#include <print>

//--------------------------------
//
//--------------------------------
VictimTemplateAuthenticator::VictimTemplateAuthenticator(std::string db_path)
    : db_path_(std::move(db_path)),
      credentials_(PasswordCredentialService::instance()) {}

//--------------------------------
//
//--------------------------------
bool VictimTemplateAuthenticator::authenticate(const std::string& username, const std::string& password) {
    VictimTemplateRepository repository(db_path_);
    const auto template_record = repository.get_username(username);

    if (!template_record.has_value()) {
        std::println("Victim template auth failed: unknown username [{}]", username);
        return false;
    }

    if (!credentials_.verify(password, template_record->password_hash)) {
        std::println("Victim template auth failed: invalid password for [{}]", username);
        return false;
    }

    Pbkdf2Sha256PasswordHasher hasher;
    if (!hasher.is_hashed(template_record->password_hash)) {
        repository.update_password_hash(template_record->id, credentials_.hash_for_storage(password));
    }

    return true;
}
