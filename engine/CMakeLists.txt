add_library(project0_engine STATIC
  track/track.cpp
  simulation/simulation.cpp
  telemetry/telemetry.cpp
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
