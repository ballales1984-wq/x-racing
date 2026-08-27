add_library(project0_engine STATIC
  track/track.cpp
  simulation/simulation.cpp
  telemetry/telemetry.cpp
  vehicle/vehicle_generator.cpp
  vehicle/car_model.cpp
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

if(PROJECT0_BUILD_ASSIMP)
  # Vendored prebuilt Assimp: an import library (lib/assimp.lib) for the
  # shared DLL (bin/assimp.dll). Consumed dynamically, so the DLL must be
  # deployed next to every executable that links this target (see below).
  target_sources(project0_engine PRIVATE assets/assimp_loader.cpp)
  add_library(assimp SHARED IMPORTED GLOBAL)
  set_target_properties(assimp PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/assimp/bin/assimp.dll"
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/assimp/lib/assimp.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/assimp/include"
  )
  target_link_libraries(project0_engine PUBLIC assimp)
  target_compile_definitions(project0_engine PUBLIC ASSIMP_AVAILABLE)
endif()

# Deploy the Assimp DLL next to a target that transitively links assimp so the
# loader resolves at runtime.
function(p0_deploy_assimp target)
  if(PROJECT0_BUILD_ASSIMP AND TARGET assimp AND WIN32)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:assimp>
        $<TARGET_FILE_DIR:${target}>
      COMMENT "Deploying assimp.dll next to ${target}")
  endif()
endfunction()

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
p0_deploy_assimp(sim_plugin)

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
