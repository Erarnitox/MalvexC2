add_library(malvex_server_services INTERFACE)

target_include_directories(malvex_server_services INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/.."
)

target_link_libraries(malvex_server_services INTERFACE managers common)
