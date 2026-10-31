cmake_minimum_required(VERSION 3.16)
project(PipewirePlayback VERSION 1.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

add_executable(pipewire_playback Main.cpp)
target_include_directories(pipewire_playback PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
target_link_directories(pipewire_playback PRIVATE ${PIPEWIRE_LIBRARY_DIRS})
target_link_libraries(pipewire_playback PRIVATE ${PIPEWIRE_LIBRARIES})
target_compile_options(pipewire_playback PRIVATE ${PIPEWIRE_CFLAGS_OTHER} -Wall -Wextra -Wpedantic)
