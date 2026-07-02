add_executable(exfil_tests exfil_tests.cpp)
target_link_libraries(exfil_tests PRIVATE common)
target_include_directories(exfil_tests PRIVATE "${CMAKE_CURRENT_LIST_DIR}/..")

add_test(NAME exfil_tests COMMAND exfil_tests)
