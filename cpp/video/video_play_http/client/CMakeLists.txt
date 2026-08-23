cmake_minimum_required(VERSION 3.16)
project(HttpVideoPlayer VERSION 1.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 第三方公共域头文件
set(MINIMP4_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ---- 依赖 ----
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
    libavcodec
    libavutil
    libswresample)
pkg_check_modules(SDL2  REQUIRED sdl2)
pkg_check_modules(CURL  REQUIRED libcurl)

add_executable(HttpVideoPlayer
    Main.cpp
    VideoHal.cpp
    VideoHal.h
)

target_include_directories(HttpVideoPlayer PRIVATE
    ${MINIMP4_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(HttpVideoPlayer PRIVATE
    ${FFMPEG_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${CURL_LIBRARIES}
    pthread
)

target_compile_options(HttpVideoPlayer PRIVATE -Wall -Wextra -Wpedantic)
