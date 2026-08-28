add_library(project0_renderer STATIC
  renderer.cpp
  dx11_renderer.cpp
)

target_include_directories(project0_renderer PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../engine
  ${CMAKE_CURRENT_SOURCE_DIR}/../vendor
)

target_compile_features(project0_renderer PUBLIC cxx_std_20)
target_link_libraries(project0_renderer PUBLIC project0_engine)

if(WIN32)
  target_link_libraries(project0_renderer PUBLIC d3d11.lib dxgi.lib d3dcompiler.lib)
endif()
