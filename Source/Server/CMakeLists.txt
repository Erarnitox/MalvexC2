add_executable(server server.cpp)

add_subdirectory(Database)
add_subdirectory(Managers)
add_subdirectory(Services)
add_subdirectory(Endpoints)
add_subdirectory(Auth)

target_link_libraries(server PRIVATE common)
target_link_libraries(server PRIVATE server_db)
target_link_libraries(server PRIVATE managers)
target_link_libraries(server PRIVATE malvex_server_endpoints)
target_link_libraries(server PRIVATE malvex_server_services)
target_link_libraries(server PRIVATE server_auth)

target_include_directories(server PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/Services"
)

target_compile_options(server
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)

add_custom_command(TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:server>
        $<TARGET_FILE_DIR:client>/$<TARGET_FILE_NAME:server>
    COMMENT "Copying server binary to client directory"
)

include(GNUInstallDirs)

install(TARGETS server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Full)
