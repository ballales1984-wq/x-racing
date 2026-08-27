add_library(project0_engine STATIC
  track/track.cpp
  simulation/simulation.cpp
  telemetry/telemetry.cpp
  vehicle/vehicle_generator.cpp
  vehicle/mesh_exporter.cpp
  vehicle/glb_exporter.cpp
  assets/mesh_loader.cpp
  input/platform/windows_input.cpp
  input/platform/auto_input.cpp
)

target_include_directories(project0_engine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../vendor
)

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
