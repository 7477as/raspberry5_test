cmake_minimum_required(VERSION 3.16)
project(AecWebrtcDsp VERSION 1.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-audio-1.0)

add_executable(aec_webrtcdsp
    Main.cpp
    AecPipeline.cpp
    DelayEstimator.cpp
    WavWriter.cpp
)

target_include_directories(aec_webrtcdsp PRIVATE ${GST_INCLUDE_DIRS})
target_link_directories(aec_webrtcdsp PRIVATE ${GST_LIBRARY_DIRS})

target_link_libraries(aec_webrtcdsp PRIVATE ${GST_LIBRARIES})

target_compile_options(aec_webrtcdsp PRIVATE
    ${GST_CFLAGS}
    -Wall
    -Wextra
    -Wpedantic
)