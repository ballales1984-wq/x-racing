cmake_minimum_required(VERSION 3.20)

set(TEST_SOURCES
  simulation_test.cpp
)

add_executable(project0_tests ${TEST_SOURCES})
target_link_libraries(project0_tests PRIVATE project0_engine GTest::gtest_main)
target_include_directories(project0_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
