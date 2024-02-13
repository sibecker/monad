cmake_minimum_required(VERSION 3.12)

project(monad)

add_library(monad INTERFACE
        monad/monad.h
        monad/optional.h
        monad/function.h
        monad/task.h
)
target_include_directories(monad INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
