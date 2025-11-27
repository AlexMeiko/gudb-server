file(GLOB CMD_SOURCES "*.cpp")

add_library(cmd OBJECT
        ${CMD_SOURCES}
)

target_include_directories(cmd PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)