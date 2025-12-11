add_executable(server server.cpp)

target_link_libraries(server PRIVATE common)

target_compile_options(server
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)