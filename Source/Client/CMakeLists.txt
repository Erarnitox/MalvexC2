set(BUILD_RAYLIB_AUDIO OFF CACHE BOOL "" FORCE)
CPMAddPackage("gh:raysan5/raylib#5.5")
CPMAddPackage("gh:raysan5/raygui#4.0")
CPMAddPackage("gh:serge1/ELFIO#Release_3.12")

add_library(raygui INTERFACE)
target_include_directories(raygui SYSTEM INTERFACE ${raygui_SOURCE_DIR}/src)
target_include_directories(raygui SYSTEM INTERFACE ${raygui_SOURCE_DIR})
target_compile_definitions(raygui INTERFACE RAYGUI_IMPLEMENTATION)

add_executable(client
    "UI.cpp"
    "Client.cpp"
    "Builder.cpp"
    "CommandManager.cpp"
    "LogManager.cpp"
    "SessionManager.cpp"
    "VictimManager.cpp"

    # Headers for IDE
    "UI.hpp"
    "Client.hpp"
    "Builder.hpp"
    "CommandManager.hpp"
    "LogManager.hpp"
    "SessionManager.hpp"
    "VictimManager.hpp"
)

target_link_libraries(client
    PRIVATE
        raylib
        raygui
        common
        elfio
)

if(UNIX AND NOT APPLE)
    target_link_libraries(client PRIVATE pthread)
endif()

target_compile_options(client
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)