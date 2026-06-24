add_library(server_auth STATIC
    OperatorAuthenticator.cpp
    VictimTemplateAuthenticator.cpp
    SessionConnectionAuthenticator.cpp

    OperatorAuthenticator.hpp
    VictimTemplateAuthenticator.hpp
    SessionConnectionAuthenticator.hpp
)

target_link_libraries(server_auth PUBLIC operator_repo)
target_link_libraries(server_auth PUBLIC victim_template_repo)
target_link_libraries(server_auth PUBLIC crypto)
target_link_libraries(server_auth PUBLIC common)
target_include_directories(server_auth PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
