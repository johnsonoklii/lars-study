file(GLOB_RECURSE LARS_REACTOR_SRCS ./src/*.cc )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lars_reactor/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lars_reactor/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lars_reactor/lib)

add_library(lreactor SHARED ${LARS_REACTOR_SRCS})

add_subdirectory(example)