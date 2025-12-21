add_library(result_repo STATIC
    ResultRepository.cpp

    # header files for IDEs
    ResultRepository.hpp
)

target_link_libraries(result_repo PRIVATE database)
target_include_directories(result_repo PUBLIC "${CMAKE_CURRENT_LIST_DIR}")