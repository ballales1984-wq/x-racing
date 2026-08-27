set(TEST_SOURCES
  simulation_test.cpp
  physics_test.cpp
  vehicle_test.cpp
  track_test.cpp
  telemetry_test.cpp
  gameplay_test.cpp
  ../game/gameplay.cpp
)

add_executable(project0_tests ${TEST_SOURCES})
target_link_libraries(project0_tests PRIVATE project0_engine GTest::gtest_main)
target_include_directories(project0_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

include(GoogleTest)
gtest_add_tests(project0_tests "" ${TEST_SOURCES})
