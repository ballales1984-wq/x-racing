add_library(project0_gameplay INTERFACE)

target_include_directories(project0_gameplay INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor
)

target_link_libraries(project0_gameplay INTERFACE project0_engine)

if(PROJECT0_BUILD_GAMEPLAY)
  add_executable(project0_gameplay_exe main.cpp gameplay.cpp)
  target_link_libraries(project0_gameplay_exe PRIVATE project0_gameplay)
  p0_deploy_assimp(project0_gameplay_exe)

  add_executable(auto_drive main_auto.cpp gameplay.cpp)
  target_link_libraries(auto_drive PRIVATE project0_gameplay)
  p0_deploy_assimp(auto_drive)

  add_executable(gen_telemetry gen_telemetry.cpp gameplay.cpp)
  target_link_libraries(gen_telemetry PRIVATE project0_gameplay)
  p0_deploy_assimp(gen_telemetry)
endif()
