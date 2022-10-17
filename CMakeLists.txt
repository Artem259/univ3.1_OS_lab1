cmake_minimum_required(VERSION 3.22)
project(univ3_1_OS_lab1)

set(CMAKE_CXX_STANDARD 20)

include_directories(.)

add_executable(univ3_1_OS_lab1
        main.cpp
        #test.cpp
        Manager.hpp
        Computation.hpp
        src/compfuncs.hpp
        src/trialfuncs.hpp)
