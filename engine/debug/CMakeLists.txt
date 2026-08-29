add_library(project0_debug STATIC
  debug_agent.cpp
  debug_console.cpp
  debug_physics.cpp
  debug_ai.cpp
  debug_network.cpp
  debug_profiler.cpp
)

target_include_directories(project0_debug PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../vendor
)

target_link_libraries(project0_debug PUBLIC project0_engine)

set_target_properties(project0_debug PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)

if(WIN32)
  target_compile_definitions(project0_debug PUBLIC _WIN32_WINNT=0x0600)
endif()
