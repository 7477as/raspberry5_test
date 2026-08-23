cmake_minimum_required(VERSION 3.16)
project(VideoEncoder VERSION 1.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)

add_executable(VideoEncoder Main.cpp)

target_include_directories(VideoEncoder PRIVATE
    ${GST_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(VideoEncoder PRIVATE
    ${GST_LIBRARIES}
    ${OpenCV_LIBS}
)
target_compile_options(VideoEncoder PRIVATE -Wall -Wextra -Wpedantic)
