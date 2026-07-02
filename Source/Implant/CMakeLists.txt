add_executable(implant
    Loader/StubMain.cpp
    Loader/StartupChecks.cpp
    Loader/ElfSelfParser.cpp
    Loader/ConfigDecryptor.cpp
    Payload/PayloadMain.cpp
    Config.cpp
    BeaconLoop.cpp
    CommandDispatcher.cpp
    ScreenshotCapture.cpp
)

target_link_libraries(implant PRIVATE common)

target_include_directories(implant PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Loader"
    "${CMAKE_CURRENT_SOURCE_DIR}/Payload"
)

option(MALVEX_PACKED "Build implant with packed stub entry" ON)
if(MALVEX_PACKED)
    target_compile_definitions(implant PRIVATE MALVEX_PACKED=1)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(implant PRIVATE pthread)
    find_package(X11)
    if(X11_FOUND)
        target_link_libraries(implant PRIVATE X11::X11)
        target_compile_definitions(implant PRIVATE MALVEX_HAS_X11=1)
    endif()
    target_link_options(implant PRIVATE "-no-pie")
endif()

target_compile_options(implant
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)

add_custom_command(TARGET implant POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:implant>
        $<TARGET_FILE_DIR:client>/$<TARGET_FILE_NAME:implant>
    COMMENT "Copying implant binary to client directory"
)

include(GNUInstallDirs)

install(TARGETS implant RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Full)
