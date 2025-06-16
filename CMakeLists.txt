cmake_minimum_required(VERSION 3.0)

project(lars-study)

# 默认编译Release版本
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    message(STATUS "No build type selected, default to Release")
endif()

option(CONET_BUILD_EXAMPLE "Build lars-study example" ON)
option(CONET_BUILD_TEST "Build lars-study test" ON)
option(ENABLE_LARS_DEBUG "Enable LARS_DEBUG macro" OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build detected, enabling debug options")
    set(ENABLE_LARS_DEBUG ON CACHE BOOL "Force ON for Debug build" FORCE)
endif()

if(ENABLE_LARS_DEBUG)
  add_definitions(-DLARS_DEBUG)
endif()

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS_DEBUG 
    "-O0 -g3 -Wall -Wextra -Werror -pthread"
    CACHE STRING "Debug compiler flags" FORCE
)

set(CMAKE_CXX_FLAGS_RELEASE
    "-O2 -g -Wall -Wextra -Werror -pthread"
    CACHE STRING "Release compiler flags" FORCE
)

# OUTPUT
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# INCLUDE
include_directories(
  ${PROJECT_SOURCE_DIR}/lars_reactor/include
  ${PROJECT_SOURCE_DIR}/lars_dns/include
  ${PROJECT_SOURCE_DIR}/lars_reporter/include
  /usr/local/include
  /usr/include
)

# LIBRARIES
link_directories(
    ${PROJECT_SOURCE_DIR}/lars_reactor/lib
    /usr/local/lib
    /usr/lib
)

add_subdirectory(lars_reactor)
add_subdirectory(lars_dns)
add_subdirectory(lars_reporter)