add_library(log_repo STATIC
    LogRepository.cpp

    # header files for IDEs
    LogRepository.hpp
)

target_link_libraries(log_repo PRIVATE database)
target_include_directories(log_repo PUBLIC "${CMAKE_CURRENT_LIST_DIR}")