cmake_minimum_required(VERSION 3.21)
project(course_plan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(course_plan course_plan.cpp)

if(MSVC)
    target_compile_options(course_plan PRIVATE /W4 /utf-8)
else()
    target_compile_options(course_plan PRIVATE -Wall -Wextra -pedantic)
    if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(course_plan PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
endif()
