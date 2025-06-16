file(GLOB_RECURSE LARS_REPORTER_TEST_SRCS ./*.cc )
file(GLOB_RECURSE LARS_PROTO_SRCS ${PROJECT_SOURCE_DIR}/base/proto/*.cc)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lars_reporter/bin)

include_directories(
    ${PROJECT_SOURCE_DIR}/base/proto
)

add_executable(lars_reporter_test1 ${LARS_REPORTER_TEST_SRCS} ${LARS_PROTO_SRCS})
target_link_libraries(lars_reporter_test1 lreactor pthread mysqlclient protobuf)