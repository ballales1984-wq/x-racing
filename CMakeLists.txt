cmake_minimum_required(VERSION 3.20)
project(Project0 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PROJECT0_BUILD_TESTS "Build tests" ON)
option(PROJECT0_BUILD_RENDERER "Build renderer" OFF)
option(PROJECT0_BUILD_EXPERIMENTS "Build experiments" ON)
option(PROJECT0_BUILD_GAMEPLAY "Build gameplay" ON)

add_subdirectory(engine)

if(PROJECT0_BUILD_TESTS)
  enable_testing()
  add_subdirectory(vendor/googletest)
  add_subdirectory(tests)
endif()

if(PROJECT0_BUILD_EXPERIMENTS)
  add_subdirectory(experiments)
endif()

if(PROJECT0_BUILD_GAMEPLAY)
  add_subdirectory(game)
endif()

if(PROJECT0_BUILD_RENDERER)
  add_subdirectory(renderer)
  add_executable(project0_app main.cpp)
  target_link_libraries(project0_app PRIVATE project0_renderer)
endif()
