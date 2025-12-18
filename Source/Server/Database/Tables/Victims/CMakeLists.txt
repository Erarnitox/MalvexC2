add_library(victim_repo STATIC
    VictimRepository.cpp

    # header files for IDEs
    VictimRepository.hpp
)

target_link_libraries(victim_repo PRIVATE database)
target_include_directories(victim_repo PUBLIC "${CMAKE_CURRENT_LIST_DIR}")