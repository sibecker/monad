cmake_minimum_required(VERSION 3.12)

project(example-headers)

add_library(example-headers INTERFACE
        example.h)
target_include_directories(example-headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

