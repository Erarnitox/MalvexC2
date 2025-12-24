add_library(endpoints INTERFACE)

target_link_libraries(endpoints INTERFACE managers)

target_link_libraries(endpoints INTERFACE common)
target_link_libraries(endpoints INTERFACE managers)

target_include_directories(endpoints INTERFACE "${CMAKE_CURRENT_LIST_DIR}")