cmake_minimum_required(VERSION 3.0)
add_compile_options("-Wall")
add_compile_options("-std=c++14")
add_executable("Client" client.cpp)
add_executable("Server" server.cpp)
target_link_libraries("Server" "pthread")
