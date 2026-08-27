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

  add_executable(track_svg track_svg.cpp ../tools/track_diagram/track_diagram.cpp)
  target_link_libraries(track_svg PRIVATE project0_experiments)
  target_include_directories(track_svg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../tools/track_diagram)
endif()
