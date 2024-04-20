cmake_minimum_required(VERSION 3.12)

project(sib/monad)

add_library(monad INTERFACE
        sib/shared_task.h
        sib/monad/monad.h
        sib/monad/optional.h
        sib/monad/function.h
        sib/monad/task.h
)
target_include_directories(monad INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
