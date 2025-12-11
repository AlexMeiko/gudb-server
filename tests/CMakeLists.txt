# 尝试查找 Catch2，如果找不到则跳过测试
find_package(Catch2 QUIET)

if (Catch2_FOUND)
    message(STATUS "Catch2 found, building tests")

    add_executable(test_gudb
            test_zskiplist.cpp
    )

    target_link_libraries(test_gudb
            PRIVATE
            Catch2::Catch2WithMain
            ds
    )

    target_include_directories(test_gudb PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )

    # 注册测试
    include(CTest)
    include(Catch)
    catch_discover_tests(test_gudb)
else ()
    message(WARNING "Catch2 not found, skipping tests. Install Catch2 to enable testing.")
    message("  Ubuntu/Debian: sudo apt-get install catch2")
    message("  Or download from: https://github.com/catchorg/Catch2")
endif ()
