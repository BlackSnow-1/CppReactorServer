cmake_minimum_required(VERSION 3.16)
project(CppReactorServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CppReactorServer main.cpp
        Client.cpp
        Client.h
        Server.cpp
        Server.h)