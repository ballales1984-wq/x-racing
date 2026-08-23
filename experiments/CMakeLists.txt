cmake_minimum_required(VERSION 3.20)
project(Project0Experiments LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(project0_experiments INTERFACE)

target_include_directories(project0_experiments INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor
)

target_link_libraries(project0_experiments INTERFACE project0_engine)

if(PROJECT0_BUILD_EXPERIMENTS)
  add_executable(track_analysis track_analysis.cpp)
  target_link_libraries(track_analysis PRIVATE project0_experiments)
endif()
