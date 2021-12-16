cmake_minimum_required(VERSION 3.21)
project(so_shell C)

set(CMAKE_C_STANDARD 11)

add_executable(so_shell main.c)

target_link_libraries(so_shell readline)