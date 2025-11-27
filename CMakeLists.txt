cmake_minimum_required(VERSION 3.25.1)
project(gudb-server)

set(CMAKE_CXX_STANDARD 20)

# 设置安装目录
include(GNUInstallDirs)

# 添加源文件目录
add_subdirectory(src)