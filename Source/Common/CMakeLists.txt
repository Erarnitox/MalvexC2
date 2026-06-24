add_library(common INTERFACE)

CPMAddPackage("gh:Erarnitox/CPPpwn#main")
target_link_libraries(common INTERFACE cpppwn::cpppwn)

target_include_directories(common INTERFACE "${CMAKE_CURRENT_LIST_DIR}")

add_subdirectory(Database)
add_subdirectory(Config)
add_subdirectory(Crypto)

target_link_libraries(common INTERFACE database)
target_link_libraries(common INTERFACE config)
target_link_libraries(common INTERFACE crypto)
target_link_libraries(common INTERFACE stdc++exp dw)
