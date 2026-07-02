set(BUILD_RAYLIB_AUDIO OFF CACHE BOOL "" FORCE)
CPMAddPackage("gh:raysan5/raylib#6.0")
CPMAddPackage("gh:raysan5/raygui#4.0")
CPMAddPackage("gh:serge1/ELFIO#Release_3.12")

add_library(raygui INTERFACE)
target_include_directories(raygui SYSTEM INTERFACE ${raygui_SOURCE_DIR}/src)
target_include_directories(raygui SYSTEM INTERFACE ${raygui_SOURCE_DIR})

add_executable(client
    "UI.cpp"
    "UiTheme.cpp"
    "UiConnectionsTab.cpp"
    "UiLogsTab.cpp"
    "UiSettingsTab.cpp"
    "UiBuilderTab.cpp"
    "UiArtifactsTab.cpp"
    "UiTerminalTab.cpp"
    "UiLogin.cpp"
    "UiAbout.cpp"
    "Client.cpp"
    "RestGateway.cpp"
    "Builder.cpp"
    "Packer/DecryptorCodegen.cpp"
    "Packer/ElfPacker.cpp"
    "Packer/ImplantPacker.cpp"
    "Packer/MetamorphicPacker.cpp"
    "CommandManager.cpp"
    "ResultManager.cpp"
    "ResultDecoders.cpp"
    "LogManager.cpp"
    "SessionManager.cpp"
    "VictimManager.cpp"

    # Headers for IDE
    "UI.hpp"
    "UiShared.hpp"
    "UiLayout.hpp"
    "Client.hpp"
    "ClientNamespaces.hpp"
    "RestGateway.hpp"
    "Builder.hpp"
    "CommandManager.hpp"
    "ResultManager.hpp"
    "ResultDecoders.hpp"
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
        -Werror
)

add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
        "$<TARGET_FILE_DIR:client>/Resources"
    COMMENT "Copying resources to client build directory..."
)

# Install interface

include(GNUInstallDirs)

install(TARGETS client RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Full)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Resources/"
        DESTINATION "${CMAKE_INSTALL_BINDIR}/Resources"
        COMPONENT Full)

if(BUILD_TESTING)
    add_executable(metamorphic_pack_smoke
        tests/pack_smoke.cpp
        Packer/DecryptorCodegen.cpp
        Packer/ElfPacker.cpp
        Packer/ImplantPacker.cpp
        Packer/MetamorphicPacker.cpp
    )
    target_link_libraries(metamorphic_pack_smoke PRIVATE common elfio)
    target_include_directories(metamorphic_pack_smoke PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME metamorphic_pack_smoke
        COMMAND metamorphic_pack_smoke "${CMAKE_BINARY_DIR}/bin/Client/implant")

    add_executable(pack_modes_test
        tests/pack_modes_test.cpp
        Packer/DecryptorCodegen.cpp
        Packer/ElfPacker.cpp
        Packer/ImplantPacker.cpp
        Packer/MetamorphicPacker.cpp
    )
    target_link_libraries(pack_modes_test PRIVATE common elfio)
    target_include_directories(pack_modes_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()