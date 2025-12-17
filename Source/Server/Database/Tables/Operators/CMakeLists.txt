add_library(operator_repo STATIC
    OperatorRepository.cpp

    # header files for IDEs
    OperatorRepository.hpp
)

target_link_libraries(operator_repo PRIVATE database)
target_include_directories(operator_repo PUBLIC "${CMAKE_CURRENT_LIST_DIR}")