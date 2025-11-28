# 添加各模块子目录
add_subdirectory(core)
add_subdirectory(protocol)
add_subdirectory(ds)
add_subdirectory(db)
add_subdirectory(cmd)
add_subdirectory(net)

# 创建主可执行文件
add_executable(gudb-server main.cpp)

# 链接所有库
target_link_libraries(gudb-server
        PRIVATE
        net
        db
        cmd
)

target_include_directories(gudb-server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 安装
install(TARGETS gudb-server
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)