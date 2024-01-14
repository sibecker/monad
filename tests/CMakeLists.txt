cmake_minimum_required(VERSION 3.12)

add_subdirectory(Catch2)

add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
target_link_libraries(tests PUBLIC example)

include(CTest)
include(Catch)
catch_discover_tests(tests)
