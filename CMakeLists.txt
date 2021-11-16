cmake_minimum_required(VERSION 3.20)
project(Fichiers C)

set(CMAKE_C_STANDARD 11)

add_executable(Sender senderUDP.c)
add_executable(Recever recvUDP.c)
target_link_libraries(Sender m)