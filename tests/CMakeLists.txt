set(TEST_SOURCES
  simulation_test.cpp
  physics_test.cpp
  lap_detector_test.cpp
  camera_test.cpp
  tire_model_test.cpp
  vehicle_test.cpp
  track_test.cpp
  track_diagram_test.cpp
  telemetry_test.cpp
  gameplay_test.cpp
  glb_exporter_test.cpp
  auto_input_test.cpp
  sim_plugin_test.cpp
  race_results_test.cpp
  common_test.cpp
  race_config_test.cpp
  track_data_test.cpp
  snapshot_test.cpp
  pit_stop_fsm_test.cpp
  pit_lane_test.cpp
  validation_test.cpp
  race_manager_test.cpp
  simulation_world_test.cpp
  result_storage_test.cpp
  tracking_test.cpp
   ai_test.cpp
   audio_test.cpp
   standings_test.cpp
   ../game/gameplay.cpp
  ../tools/track_diagram/track_diagram.cpp
)

set(NON_TEST_SOURCES
  ../game/gameplay.cpp
  ../tools/track_diagram/track_diagram.cpp
)

if(PROJECT0_BUILD_ASSIMP)
  list(APPEND TEST_SOURCES assimp_loader_test.cpp)
endif()

add_executable(project0_tests ${TEST_SOURCES})
target_link_libraries(project0_tests PRIVATE project0_engine GTest::gtest_main)
target_include_directories(project0_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../tools/track_diagram)

if(PROJECT0_BUILD_ASSIMP)
  deploy_assimp_dll(project0_tests)
endif()

if(BUILD_SHARED_LIBS)
  add_custom_command(TARGET project0_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:gtest>
      $<TARGET_FILE_DIR:project0_tests>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:gtest_main>
      $<TARGET_FILE_DIR:project0_tests>
    COMMENT "Deploying gtest runtime for project0_tests"
  )
endif()

include(GoogleTest)
add_test(NAME project0_tests COMMAND project0_tests)
set_tests_properties(project0_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
