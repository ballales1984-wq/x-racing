add_library(project0_tools INTERFACE)

target_include_directories(project0_tools INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor
)

target_link_libraries(project0_tools INTERFACE project0_engine)

add_executable(track_diagram
  track_diagram/track_diagram.cpp
  track_diagram/main.cpp
)
target_link_libraries(track_diagram PRIVATE project0_tools)

if(PROJECT0_BUILD_ASSIMP)
  deploy_assimp_dll(track_diagram)
endif()
