#pragma once

#include <VictimTemplateRepository.hpp>
#include <PasswordCredentialService.hpp>

#include <string>

//--------------------------------
//
//--------------------------------
class IVictimTemplateAuthenticator {
public:
    virtual ~IVictimTemplateAuthenticator() = default;
    [[nodiscard]] virtual bool authenticate(const std::string& username, const std::string& password) = 0;
};

//--------------------------------
//
//--------------------------------
class VictimTemplateAuthenticator final : public IVictimTemplateAuthenticator {
public:
    explicit VictimTemplateAuthenticator(std::string db_path);

    [[nodiscard]] bool authenticate(const std::string& username, const std::string& password) override;

private:
    std::string db_path_;
    PasswordCredentialService& credentials_;
};
