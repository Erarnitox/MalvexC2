add_library(command_repo STATIC
    CommandRepository.cpp

    # header files for IDEs
    CommandRepository.hpp
)

target_link_libraries(command_repo PRIVATE database)
target_include_directories(command_repo PUBLIC "${CMAKE_CURRENT_LIST_DIR}")