set(BUILD_RAYLIB_AUDIO OFF CACHE BOOL "" FORCE)
CPMAddPackage("gh:raysan5/raylib#5.5")
CPMAddPackage("gh:raysan5/raygui#4.0")

add_library(raygui INTERFACE)
target_include_directories(raygui SYSTEM INTERFACE ${raygui_SOURCE_DIR}/src)
target_include_directories(raygui SYSTEM INTERFACE ${raygui_SOURCE_DIR})
target_compile_definitions(raygui INTERFACE RAYGUI_IMPLEMENTATION)

add_executable(client client.cpp)

target_link_libraries(client
    PRIVATE
        raylib
        raygui
        common
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