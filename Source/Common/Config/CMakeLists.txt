add_library(config STATIC
    ConfigRepository.cpp

    # header files for IDEs
    Config.hpp
    ConfigRepository.hpp
    ConfigDAO.hpp
)

target_link_libraries(config PUBLIC database)
target_include_directories(config PUBLIC "${CMAKE_CURRENT_LIST_DIR}")