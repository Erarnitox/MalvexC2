#pragma once

#include <OperatorRepository.hpp>
#include <PasswordCredentialService.hpp>

#include <string>

//--------------------------------
//
//--------------------------------
class IOperatorAuthenticator {
public:
    virtual ~IOperatorAuthenticator() = default;
    [[nodiscard]] virtual bool authenticate(const std::string& username, const std::string& password) = 0;
};

//--------------------------------
//
//--------------------------------
class OperatorAuthenticator final : public IOperatorAuthenticator {
public:
    explicit OperatorAuthenticator(std::string db_path);

    [[nodiscard]] bool authenticate(const std::string& username, const std::string& password) override;

private:
    std::string db_path_;
    PasswordCredentialService& credentials_;
};
