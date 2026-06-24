CPMAddPackage(
  NAME SQLiteCpp
  GITHUB_REPOSITORY SRombauts/SQLiteCpp
  GIT_TAG "master"
  OPTIONS
    "SQLITECPP_INTERNAL_SQLITE ON"
    "SQLITECPP_ENABLE_TESTS OFF"
)

add_library(database STATIC
    "Database.cpp"
    "OperatorDAO.cpp"
    "VictimTemplateDAO.cpp"

    # headers for IDEs
    "Database.hpp"
    "IDao.hpp"
    "IRepository.hpp"
)

target_link_libraries(database PUBLIC SQLiteCpp)
target_link_libraries(database PUBLIC glaze::glaze)
target_include_directories(database PUBLIC "${CMAKE_CURRENT_LIST_DIR}")