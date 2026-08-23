cmake_minimum_required(VERSION 3.16)
project(CameraPreview VERSION 1.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENCV4 REQUIRED opencv4)

add_executable(CameraPreview
    Main.cpp
    CameraPreview.cpp
    CameraPreview.h
)

target_include_directories(CameraPreview PRIVATE ${OPENCV4_INCLUDE_DIRS})
target_link_libraries(CameraPreview PRIVATE ${OPENCV4_LIBRARIES})
target_compile_options(CameraPreview PRIVATE -Wall -Wextra -Wpedantic)
