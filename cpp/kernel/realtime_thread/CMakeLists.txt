cmake_minimum_required(VERSION 3.16)
project(RealtimeThread VERSION 1.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(RealtimeThread
    Main.cpp
    RealtimeThread.cpp
    RealtimeThread.h
)

target_compile_options(RealtimeThread PRIVATE -Wall -Wextra -Wpedantic)

# realtime 程序需要 realtime 权限，运行时需 sudo
install(TARGETS RealtimeThread RUNTIME DESTINATION bin)
