add_library(common INTERFACE)

CPMAddPackage("gh:Erarnitox/CPPpwn#main")
target_link_libraries(common INTERFACE cpppwn::cpppwn)

CPMAddPackage(
  NAME cpp-base64
  GITHUB_REPOSITORY ReneNyffenegger/cpp-base64
  GIT_TAG master
  DOWNLOAD_ONLY YES
)

# Create a library target manually
if(cpp-base64_ADDED)
  add_library(cpp_base64 STATIC
    ${cpp-base64_SOURCE_DIR}/base64.cpp
  )

  target_include_directories(cpp_base64 PUBLIC
    ${cpp-base64_SOURCE_DIR}
  )

  target_compile_features(cpp_base64 PUBLIC cxx_std_11)
endif()

target_link_libraries(common INTERFACE cpp_base64)

target_include_directories(common INTERFACE "${CMAKE_CURRENT_LIST_DIR}")

add_subdirectory(Database)
add_subdirectory(Config)

target_link_libraries(common INTERFACE database)
target_link_libraries(common INTERFACE config)
