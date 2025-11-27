# Database 库是头文件-only 实现
add_library(db INTERFACE)

target_link_libraries(db INTERFACE ds)
target_include_directories(db INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)