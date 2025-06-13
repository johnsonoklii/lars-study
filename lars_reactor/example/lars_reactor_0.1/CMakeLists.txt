add_executable(server server.cc)
target_link_libraries(server lreactor)

add_executable(client client.cc)
target_link_libraries(client lreactor)