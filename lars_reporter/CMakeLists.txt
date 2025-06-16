file(GLOB_RECURSE LARS_REPORTER_SRCS ./src/*.cc )
file(GLOB_RECURSE LARS_PROTO_SRCS ${PROJECT_SOURCE_DIR}/base/proto/*.cc)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lars_reporter/bin)

include_directories(
    ${PROJECT_SOURCE_DIR}/base/proto
)

add_executable(lars_reporter ${LARS_REPORTER_SRCS} ${LARS_PROTO_SRCS})
target_link_libraries(lars_reporter lreactor pthread mysqlclient protobuf)


add_subdirectory(test)