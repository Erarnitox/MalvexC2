add_executable(server server.cpp)

add_subdirectory(Database)

target_link_libraries(server PRIVATE common)
target_link_libraries(server PRIVATE server_db)

target_compile_options(server
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)