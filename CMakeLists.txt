cmake_minimum_required(VERSION 3.6)

project(shk-asm)

add_executable(shk-asm src/main.cpp)
target_include_directories(shk-asm PRIVATE include ../shk/include)
set_property(TARGET shk-asm PROPERTY CXX_STANDARD 17)
set_property(TARGET shk-asm PROPERTY CXX_STANDARD_REQUIRED ON)
