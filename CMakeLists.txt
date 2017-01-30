cmake_minimum_required(VERSION 3.1)
project(crt)

find_package(Threads REQUIRED)

add_executable(crt main.cpp chrono.cpp chrono.h)
target_link_libraries(crt Threads::Threads)


