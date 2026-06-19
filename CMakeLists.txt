cmake_minimum_required(VERSION 3.28)
project(untitled)

set(CMAKE_CXX_STANDARD 17)

add_executable(untitled main.cpp
        notification.h
        notification.cpp)

target_include_directories(untitled PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(untitled PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/libraylib.a"
        opengl32
        gdi32
        winmm
)