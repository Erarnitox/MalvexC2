cmake_minimum_required(VERSION 3.30)

project(malvex
	VERSION 1.0.0
	DESCRIPTION "A simple C2 Framework in modern C++"
	HOMEPAGE_URL "https://erarnitox.de"
	LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/CPM.cmake")
  file(DOWNLOAD
    "https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/cpm.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/cmake/CPM.cmake"
    STATUS status
    SHOW_PROGRESS
  )
  list(GET status 0 status_code)
  if(NOT status_code EQUAL 0)
    message(FATAL_ERROR "Could not download CPM.cmake")
  endif()
endif()

include(cmake/CPM.cmake)

option(BUILD_TESTING "Build unit tests" ON)

# disable in source builds
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(FATAL_ERROR "In source builds are not supported!")
endif()

# Get the LOG_LEVEL from CMake (default to INFO if not set)
set(LOG_LEVEL "INFO" CACHE STRING "Level of logging")

if(LOG_LEVEL STREQUAL "DEBUG")
    add_compile_definitions(LOG_LEVEL_DEBUG)
elseif(LOG_LEVEL STREQUAL "WARN")
    add_compile_definitions(LOG_LEVEL_WARN)
elseif(LOG_LEVEL STREQUAL "ERROR")
    add_compile_definitions(LOG_LEVEL_ERROR)
elseif(LOG_LEVEL STREQUAL "NONE")
    add_compile_definitions(LOG_LEVEL_NONE)
else()
    add_compile_definitions(LOG_LEVEL_INFO)
endif()

add_subdirectory(Source bin)

# CPack Configuration
set(CPACK_PACKAGE_NAME "MalvexC2")
set(CPACK_PACKAGE_VENDOR "Erarnitox")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern C++23 C2 Framework")
set(CPACK_PACKAGE_VERSION "0.0.4")

set(CPACK_GENERATOR "ZIP")

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL Full)

set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MalvexC2-${CPACK_PACKAGE_VERSION}")

include(CPack)