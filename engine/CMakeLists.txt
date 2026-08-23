add_library(project0_engine STATIC
  track/track.cpp
  simulation/simulation.cpp
  telemetry/telemetry.cpp
)

target_include_directories(project0_engine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../vendor
)

target_compile_features(project0_engine PUBLIC cxx_std_20)
