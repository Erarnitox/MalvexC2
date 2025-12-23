add_library(victim_template_repo STATIC
    VictimTemplateRepository.cpp

    # header files for IDEs
    VictimTemplateRepository.hpp
)

target_link_libraries(victim_template_repo PRIVATE database)
target_include_directories(victim_template_repo PUBLIC "${CMAKE_CURRENT_LIST_DIR}")