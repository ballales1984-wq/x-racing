add_library(project0_engine STATIC
  track/track.cpp
  track/track_data.h
  track/race_config.h
  track/pit_lane.cpp
  track/pit_stop_fsm.cpp
  track/validation.cpp
  track/race_manager.cpp
  track/race_results.cpp
  track/result_storage.cpp
  simulation/simulation.cpp
  simulation/simulation_world.cpp
  telemetry/telemetry.cpp
  vehicle/vehicle_generator.cpp
  vehicle/car_model.cpp
  vehicle/mesh_exporter.cpp
  vehicle/glb_exporter.cpp
  assets/mesh_loader.cpp
  assets/gltf_loader.cpp
  input/platform/windows_input.cpp
  input/platform/auto_input.cpp
  network/network_session.cpp
  network/lobby.cpp
  ai/ai_driver.cpp
)

target_include_directories(project0_engine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../vendor
)

if(PROJECT0_BUILD_ASSIMP)
  target_sources(project0_engine PRIVATE assets/assimp_loader.cpp)
  target_include_directories(project0_engine PUBLIC ${ASSIMP_INCLUDE_DIR})
  target_link_libraries(project0_engine PUBLIC ${ASSIMP_LIBRARY})
  target_compile_definitions(project0_engine PUBLIC ASSIMP_AVAILABLE)

  function(deploy_assimp_dll target)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ASSIMP_DLL}
        $<TARGET_FILE_DIR:${target}>
      COMMENT "Deploying Assimp runtime for ${target}"
    )
  endfunction()
endif()

set_target_properties(project0_engine PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

if(WIN32)
  target_compile_definitions(project0_engine PUBLIC _WIN32_WINNT=0x0600)
endif()

add_library(sim_plugin SHARED
  plugin/sim_plugin.cpp
)

target_include_directories(sim_plugin PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../vendor
)

target_link_libraries(sim_plugin PRIVATE project0_engine)

set_target_properties(sim_plugin PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  PREFIX ""
  SUFFIX ".dll"
)

if(PROJECT0_BUILD_ASSIMP)
  deploy_assimp_dll(sim_plugin)
endif()

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../UnityProject/Assets/Plugins")
  add_custom_command(TARGET sim_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:sim_plugin>
      "${CMAKE_CURRENT_SOURCE_DIR}/../UnityProject/Assets/Plugins/sim_plugin.dll"
    COMMENT "Deploying sim_plugin.dll to Unity project"
  )
  if(PROJECT0_BUILD_ASSIMP)
    add_custom_command(TARGET sim_plugin POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ASSIMP_DLL}
        "${CMAKE_CURRENT_SOURCE_DIR}/../UnityProject/Assets/Plugins"
      COMMENT "Deploying Assimp runtime to Unity project"
    )
  endif()
endif()

# Vehicle generator tool
add_executable(generate_vehicle
  tools/generate_vehicle.cpp
)

target_include_directories(generate_vehicle PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../vendor
)

target_link_libraries(generate_vehicle PRIVATE project0_engine)

if(PROJECT0_BUILD_ASSIMP)
  deploy_assimp_dll(generate_vehicle)
endif()
