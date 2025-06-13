add_executable(qps_server qps_server.cc echoMessage.pb.cc)
target_link_libraries(qps_server lreactor protobuf)

add_executable(qps_client qps_client.cc echoMessage.pb.cc)
target_link_libraries(qps_client lreactor protobuf)