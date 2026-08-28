cmake_minimum_required(VERSION 3.20)
project(Project0 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PROJECT0_BUILD_TESTS "Build tests" ON)
option(PROJECT0_BUILD_RENDERER "Build renderer" OFF)
option(PROJECT0_BUILD_EXPERIMENTS "Build experiments" ON)
option(PROJECT0_BUILD_GAMEPLAY "Build gameplay" ON)
option(PROJECT0_BUILD_ASSIMP "Build with Assimp importer" OFF)

if(PROJECT0_BUILD_ASSIMP)
  include(FetchContent)
  FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.3
  )
  # Build only the importer library; disable tests/tools/samples and
  # bundle zlib so there is no external dependency.
  set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
  set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(assimp)
endif()

add_subdirectory(engine)

if(PROJECT0_BUILD_TESTS)
  enable_testing()
  set(GOOGLETEST_VERSION 1.16.0)
  add_subdirectory(vendor/googletest/googletest)
  add_subdirectory(tests)
endif()

if(PROJECT0_BUILD_EXPERIMENTS)
  add_subdirectory(experiments)
endif()

if(PROJECT0_BUILD_GAMEPLAY)
  add_subdirectory(game)
endif()

add_subdirectory(tools)

if(PROJECT0_BUILD_RENDERER)
  add_subdirectory(renderer)
  add_executable(project0_app main.cpp)
  target_link_libraries(project0_app PRIVATE project0_renderer project0_engine)
endif()
