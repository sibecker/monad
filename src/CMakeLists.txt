cmake_minimum_required(VERSION 3.12)

add_library(example STATIC
        example.cpp
)
target_link_libraries(example PUBLIC example-headers)
