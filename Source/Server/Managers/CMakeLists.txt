add_library(managers INTERFACE)

target_link_libraries(managers INTERFACE common)
target_link_libraries(managers INTERFACE server_db)

target_include_directories(managers INTERFACE "${CMAKE_CURRENT_LIST_DIR}")