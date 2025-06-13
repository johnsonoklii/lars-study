file(GLOB_RECURSE LARS_REPORTER_SRCS ./src/*.cc )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lars_reporter/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lars_reporter/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lars_reporter/lib)

add_library(lreactor SHARED ${LARS_REPORTER_SRCS})

add_subdirectory(example)