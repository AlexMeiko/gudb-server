# 创建 net 静态库
add_library(net STATIC
        Server.cpp
        Connection.cpp
)

# 链接依赖库
target_link_libraries(net PUBLIC
        core
        protocol
        db
)

# 设置头文件搜索路径
target_include_directories(net PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)
