cmake_minimum_required(VERSION 3.16)
project(C__ReactorServer)

set(CMAKE_CXX_STANDARD 11)

add_executable(C__ReactorServer main.cpp
        Client.cpp
        Client.h
        Server.cpp
        Server.h)