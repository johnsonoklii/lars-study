file(GLOB_RECURSE LARS_DNS_SRCS ./src/*.cc )
file(GLOB_RECURSE LARS_PROTO_SRCS ${PROJECT_SOURCE_DIR}/base/proto/*.cc)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lars_dns/bin)

include_directories(
    ${PROJECT_SOURCE_DIR}/base/proto
)

add_executable(lars_dns ${LARS_DNS_SRCS} ${LARS_PROTO_SRCS})
target_link_libraries(lars_dns lreactor pthread mysqlclient protobuf)


add_subdirectory(test)