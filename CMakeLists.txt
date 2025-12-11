cmake_minimum_required(VERSION 3.12.0)
project(gudb-server)

set(CMAKE_CXX_STANDARD 20)

# 静态编译选项
option(ENABLE_STATIC "Enable static compilation" OFF)
if (ENABLE_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    message(STATUS "Static compilation enabled")
endif ()

# 默认构建类型为 Release
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 设置安装目录
include(GNUInstallDirs)

# 添加源文件目录
add_subdirectory(src)

# 单元测试
add_subdirectory(tests)
