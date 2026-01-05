add_executable(server server.cpp)

add_subdirectory(Database)
add_subdirectory(Managers)
add_subdirectory(Endpoints)

target_link_libraries(server PRIVATE common)
target_link_libraries(server PRIVATE server_db)
target_link_libraries(server PRIVATE managers)
target_link_libraries(server PRIVATE endpoints)

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

# Install interface

include(GNUInstallDirs)

install(TARGETS server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Full)