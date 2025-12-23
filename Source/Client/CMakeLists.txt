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

add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
        "$<TARGET_FILE_DIR:client>/Resources"
    COMMENT "Copying resources to client build directory..."
)

# Install interface

include(GNUInstallDirs)

install(TARGETS client RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Resources/"
        DESTINATION "${CMAKE_INSTALL_BINDIR}/Resources")