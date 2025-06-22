add_executable(udp_server udp_server.cc)
target_link_libraries(udp_server lreactor)

add_executable(udp_client udp_client.cc)
target_link_libraries(udp_client lreactor)