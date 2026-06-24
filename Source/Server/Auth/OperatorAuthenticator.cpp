#include "OperatorAuthenticator.hpp"

#include <Pbkdf2Sha256PasswordHasher.hpp>

//--------------------------------
//
//--------------------------------
OperatorAuthenticator::OperatorAuthenticator(std::string db_path)
    : db_path_(std::move(db_path)),
      credentials_(PasswordCredentialService::instance()) {}

//--------------------------------
//
//--------------------------------
bool OperatorAuthenticator::authenticate(const std::string& username, const std::string& password) {
    OperatorRepository repository(db_path_);
    const auto operator_record = repository.get_username(username);

    if (!operator_record.has_value()) {
        return false;
    }

    if (!credentials_.verify(password, operator_record->password_hash)) {
        return false;
    }

    Pbkdf2Sha256PasswordHasher hasher;
    if (!hasher.is_hashed(operator_record->password_hash)) {
        repository.update_password_hash(operator_record->id, credentials_.hash_for_storage(password));
    }

    return true;
}
