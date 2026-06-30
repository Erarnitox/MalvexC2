add_executable(implant
    implant.cpp
    BeaconLoop.cpp
    CommandDispatcher.cpp
)

target_link_libraries(implant PRIVATE common)

if(UNIX AND NOT APPLE)
    target_link_libraries(implant PRIVATE pthread)
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
