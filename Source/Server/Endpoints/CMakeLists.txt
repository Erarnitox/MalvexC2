add_library(malvex_server_endpoints STATIC
    Endpoints.cpp
    BeaconEndpoint.cpp
)

target_link_libraries(malvex_server_endpoints
    PUBLIC
        managers
        malvex_server_services
)

target_include_directories(malvex_server_endpoints
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/.."
)
