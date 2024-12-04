cmake_minimum_required(VERSION 3.22)
project(
  groupchat
  VERSION 1.0
  DESCRIPTION "Simple group chat")
add_executable(server src/inet_server.c src/queue.c)
add_executable(client src/client.c)
add_compile_options(-lrt, -lpthread)
include_directories(include)
