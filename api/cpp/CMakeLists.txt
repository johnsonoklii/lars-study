file(GLOB_RECURSE LARS_API_SRCS ./lars_api/*.cc )
file(GLOB_RECURSE LARS_PROTO_SRCS ${PROJECT_SOURCE_DIR}/base/proto/*.cc)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/api/cpp/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/api/cpp/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/api/cpp/lib)

include_directories(
    ${PROJECT_SOURCE_DIR}/base/proto
)

add_library(llarsclient SHARED ${LARS_API_SRCS} ${LARS_PROTO_SRCS})

add_subdirectory(example)