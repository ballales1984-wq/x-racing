set(TEST_SOURCES
  simulation_test.cpp
  physics_test.cpp
  vehicle_test.cpp
  track_test.cpp
  track_diagram_test.cpp
  telemetry_test.cpp
  gameplay_test.cpp
  glb_exporter_test.cpp
  auto_input_test.cpp
  sim_plugin_test.cpp
  ../game/gameplay.cpp
  ../tools/track_diagram/track_diagram.cpp
)

if(PROJECT0_BUILD_ASSIMP)
  list(APPEND TEST_SOURCES assimp_loader_test.cpp)
endif()

add_executable(project0_tests ${TEST_SOURCES})
target_link_libraries(project0_tests PRIVATE project0_engine GTest::gtest_main)
target_include_directories(project0_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../tools/track_diagram)

include(GoogleTest)
gtest_add_tests(project0_tests "" ${TEST_SOURCES})
