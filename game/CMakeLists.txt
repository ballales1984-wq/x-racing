cmake_minimum_required(VERSION 3.20)
project(Project0Gameplay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

  add_executable(auto_drive main_auto.cpp gameplay.cpp)
  target_link_libraries(auto_drive PRIVATE project0_gameplay)

  add_executable(gen_telemetry gen_telemetry.cpp gameplay.cpp)
  target_link_libraries(gen_telemetry PRIVATE project0_gameplay)
endif()
