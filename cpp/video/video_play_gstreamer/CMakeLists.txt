cmake_minimum_required(VERSION 3.16)
project(GstMp4Player VERSION 1.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)

find_package(Threads REQUIRED)

add_executable(GstMp4Player Main.cpp)

target_include_directories(GstMp4Player PRIVATE ${GST_INCLUDE_DIRS})
target_link_directories(GstMp4Player PRIVATE ${GST_LIBRARY_DIRS})

target_link_libraries(GstMp4Player PRIVATE
    ${GST_LIBRARIES}
    Threads::Threads
)

target_compile_options(GstMp4Player PRIVATE
    ${GST_CFLAGS}
    -Wall -Wextra -Wpedantic
)
