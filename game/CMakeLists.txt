add_library(project0_gameplay INTERFACE)

target_include_directories(project0_gameplay INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor
)

target_link_libraries(project0_gameplay INTERFACE project0_engine project0_debug)

if(PROJECT0_BUILD_GAMEPLAY)
  add_executable(project0_gameplay_exe main.cpp gameplay.cpp multiplayer_gameplay.cpp)
  target_link_libraries(project0_gameplay_exe PRIVATE project0_gameplay)

  add_executable(auto_drive main_auto.cpp gameplay.cpp)
  target_link_libraries(auto_drive PRIVATE project0_gameplay)

  add_executable(gen_telemetry gen_telemetry.cpp gameplay.cpp)
  target_link_libraries(gen_telemetry PRIVATE project0_gameplay)

  add_executable(debug_agent main_debug.cpp)
  target_link_libraries(debug_agent PRIVATE project0_gameplay)

  if(PROJECT0_BUILD_ASSIMP)
    deploy_assimp_dll(project0_gameplay_exe)
    deploy_assimp_dll(auto_drive)
    deploy_assimp_dll(gen_telemetry)
    deploy_assimp_dll(debug_agent)
  endif()
endif()
