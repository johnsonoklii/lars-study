include_directories(../lars_api)
link_directories(../lib)

add_executable(api_client api_client.cc)
target_link_libraries(api_client lreactor llarsclient protobuf)