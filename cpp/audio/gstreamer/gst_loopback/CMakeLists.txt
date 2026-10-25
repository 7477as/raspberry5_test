cmake_minimum_required(VERSION 3.16)
project(GstAudioLoopback VERSION 1.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)

add_executable(gst_loopback Main.cpp)

target_include_directories(gst_loopback PRIVATE ${GST_INCLUDE_DIRS})
target_link_directories(gst_loopback PRIVATE ${GST_LIBRARY_DIRS})

target_link_libraries(gst_loopback PRIVATE ${GST_LIBRARIES})

target_compile_options(gst_loopback PRIVATE
    ${GST_CFLAGS}
    -Wall
    -Wextra
    -Wpedantic
)
