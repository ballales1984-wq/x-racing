add_library(project0_engine STATIC
  track/track.cpp
  simulation/simulation.cpp
  telemetry/telemetry.cpp
  vehicle/vehicle_generator.cpp
  vehicle/car_model.cpp
  vehicle/mesh_exporter.cpp
  vehicle/glb_exporter.cpp
  assets/mesh_loader.cpp
  assets/gltf_loader.cpp
  input/platform/windows_input.cpp
  input/platform/auto_input.cpp
)

target_include_directories(project0_engine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../vendor
)

if(PROJECT0_BUILD_ASSIMP)
  # The `assimp` target is provided by FetchContent in the root CMakeLists.
  target_sources(project0_engine PRIVATE assets/assimp_loader.cpp)
  target_link_libraries(project0_engine PUBLIC assimp)
  target_compile_definitions(project0_engine PUBLIC ASSIMP_AVAILABLE)
endif()

target_compile_features(project0_engine PUBLIC cxx_std_20)

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

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../UnityProject/Assets/Plugins")
  add_custom_command(TARGET sim_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:sim_plugin>
      "${CMAKE_CURRENT_SOURCE_DIR}/../../UnityProject/Assets/Plugins/sim_plugin.dll"
    COMMENT "Deploying sim_plugin.dll to Unity project"
  )
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
