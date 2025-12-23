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
)