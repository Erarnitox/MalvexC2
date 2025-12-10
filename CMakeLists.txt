cmake_minimum_required(VERSION 4.2)

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

# disable in source builds
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(FATAL_ERROR "In source builds are not supported!")
endif()

add_subdirectory(Source bin)