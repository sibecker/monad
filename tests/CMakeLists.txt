cmake_minimum_required(VERSION 3.12)

add_subdirectory(Catch2)

add_executable(tests
        optional.cpp
        function.cpp
        task.cpp
        shared_task.cpp
)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
target_link_libraries(tests PUBLIC monad)

include(CTest)
include(Catch)
catch_discover_tests(tests)
