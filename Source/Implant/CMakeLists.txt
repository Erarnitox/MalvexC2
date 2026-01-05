add_executable(implant implant.cpp)

target_link_libraries(implant PRIVATE common)

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

# Install interface

include(GNUInstallDirs)

install(TARGETS implant RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Full)