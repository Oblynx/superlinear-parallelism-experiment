cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_COMPILER g++)
project(superlinParallel)

add_compile_options(-O1 -march=native)

add_executable(randomWalks src/randomWalks.cpp)

