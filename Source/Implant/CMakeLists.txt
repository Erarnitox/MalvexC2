add_executable(implant implant.cpp)

target_link_libraries(implant PRIVATE common)

target_compile_options(implant
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)