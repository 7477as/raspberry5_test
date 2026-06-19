cmake_minimum_required(VERSION 3.10)
project(GstH264Test)

set(CMAKE_CXX_STANDARD 17)

# 查找 OpenCV
find_package(OpenCV REQUIRED)

# 查找 GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)

add_executable(test main.cpp)

# 包含头文件与链接库
target_include_directories(test PRIVATE ${GST_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(test ${GST_LIBRARIES} ${OpenCV_LIBS})