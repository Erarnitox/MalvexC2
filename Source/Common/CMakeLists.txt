add_library(common INTERFACE)

CPMAddPackage("gh:Erarnitox/CPPpwn#main")
target_link_libraries(common INTERFACE cpppwn::cpppwn)

CPMAddPackage(
  NAME SQLiteCpp
  GITHUB_REPOSITORY SRombauts/SQLiteCpp
  GIT_TAG "master"
  OPTIONS
    "SQLITECPP_INTERNAL_SQLITE ON"
    "SQLITECPP_ENABLE_TESTS OFF"
)
target_link_libraries(common INTERFACE SQLiteCpp)