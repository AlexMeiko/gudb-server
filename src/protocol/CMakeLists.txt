add_library(protocol STATIC
        Parser.cpp
        Encoder.cpp
)

target_link_libraries(protocol PUBLIC core)
target_include_directories(protocol PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)