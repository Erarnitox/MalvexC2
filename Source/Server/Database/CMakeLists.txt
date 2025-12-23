add_library(server_db INTERFACE)

target_include_directories(server_db INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}"
)

add_subdirectory(Tables)

target_link_libraries(server_db INTERFACE operator_repo)
target_link_libraries(server_db INTERFACE victim_repo)
target_link_libraries(server_db INTERFACE command_repo)
target_link_libraries(server_db INTERFACE log_repo)
target_link_libraries(server_db INTERFACE result_repo)
target_link_libraries(server_db INTERFACE session_repo)
target_link_libraries(server_db INTERFACE victim_template_repo)