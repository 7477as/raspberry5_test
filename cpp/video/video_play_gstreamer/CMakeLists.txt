cmake_minimum_required(VERSION 3.10)

# 定义项目名称
project(GstMp4Player)

# 强制使用 C++11 标准 (因为代码中用到了 std::thread)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找 PkgConfig 工具
find_package(PkgConfig REQUIRED)

# 使用 pkg-config 查找 gstreamer-1.0 库
pkg_check_modules(GST REQUIRED gstreamer-1.0)

# 查找系统的线程库 (pthread)
find_package(Threads REQUIRED)

# 添加可执行文件目标
add_executable(player player.cpp)

# 包含 GStreamer 的头文件目录
target_include_directories(player PRIVATE ${GST_INCLUDE_DIRS})

# 包含 GStreamer 的链接库目录
target_link_directories(player PRIVATE ${GST_LIBRARY_DIRS})

# 链接 GStreamer 核心库 和 线程库
target_link_libraries(player PRIVATE ${GST_LIBRARIES} Threads::Threads)

# 附加 GStreamer 相关的编译选项
target_compile_options(player PRIVATE ${GST_CFLAGS})