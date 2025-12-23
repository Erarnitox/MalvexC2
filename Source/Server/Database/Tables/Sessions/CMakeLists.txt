add_library(session_repo STATIC
    SessionRepository.cpp

    # header files for IDEs
    SessionRepository.hpp
)

target_link_libraries(session_repo PRIVATE database)
target_include_directories(session_repo PUBLIC "${CMAKE_CURRENT_LIST_DIR}")