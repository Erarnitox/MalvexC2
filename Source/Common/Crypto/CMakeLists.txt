find_package(OpenSSL REQUIRED)

add_library(crypto STATIC
    Pbkdf2Sha256PasswordHasher.cpp
    PasswordCredentialService.cpp

    IPasswordHasher.hpp
    Pbkdf2Sha256PasswordHasher.hpp
    PasswordCredentialService.hpp
)

target_link_libraries(crypto PUBLIC OpenSSL::Crypto)
target_include_directories(crypto PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

target_compile_options(crypto
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)
