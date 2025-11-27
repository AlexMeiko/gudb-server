add_library(ds STATIC
        ZSkipList.cpp
)

target_include_directories(ds PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)
