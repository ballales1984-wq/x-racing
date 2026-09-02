cmake_minimum_required(VERSION 3.20)
project(xe_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /EHsc)
endif()

# ---- xe_core: engine runtime library (no editor/tools) ----
add_library(xe_core STATIC
    core/logger.cpp
    core/clock.cpp
    core/application.cpp
    platform/win32/win32_window.cpp
    platform/win32/win32_input.cpp
    rendering/dx12/dx12_renderer.cpp
)

target_include_directories(xe_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(xe_core PUBLIC cxx_std_20)

if(WIN32)
    target_compile_definitions(xe_core PUBLIC UNICODE _UNICODE _WIN32_WINNT=0x0600)
    target_link_libraries(xe_core PUBLIC user32.lib d3d12.lib dxgi.lib)

    set(XE_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rendering/dx12/shaders)
    set(XE_FXC_PATH "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe")
    if(EXISTS "${XE_FXC_PATH}")
        set(FXC_COMPILER "${XE_FXC_PATH}")
    else()
        find_program(FXC_COMPILER fxc)
    endif()
    if(FXC_COMPILER)
        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/X-Engine/shaders/triangle_vs.cso
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/X-Engine/shaders
            COMMAND ${FXC_COMPILER} /T vs_5_0 /E main /Fo ${CMAKE_BINARY_DIR}/X-Engine/shaders/triangle_vs.cso ${XE_SHADER_DIR}/triangle_vs.hlsl
            DEPENDS ${XE_SHADER_DIR}/triangle_vs.hlsl
            COMMENT "Compiling triangle_vs.hlsl"
        )
        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/X-Engine/shaders/triangle_ps.cso
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/X-Engine/shaders
            COMMAND ${FXC_COMPILER} /T ps_5_0 /E main /Fo ${CMAKE_BINARY_DIR}/X-Engine/shaders/triangle_ps.cso ${XE_SHADER_DIR}/triangle_ps.hlsl
            DEPENDS ${XE_SHADER_DIR}/triangle_ps.hlsl
            COMMENT "Compiling triangle_ps.hlsl"
        )
        add_custom_target(xe_shaders
            DEPENDS ${CMAKE_BINARY_DIR}/X-Engine/shaders/triangle_vs.cso ${CMAKE_BINARY_DIR}/X-Engine/shaders/triangle_ps.cso
        )
    endif()
endif()

# ---- xengine_runtime: standalone runtime executable ----
add_executable(xengine_runtime main.cpp)
target_link_libraries(xengine_runtime PRIVATE xe_core)
if(WIN32 AND FXC_COMPILER)
    add_dependencies(xengine_runtime xe_shaders)
    set_target_properties(xengine_runtime PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/X-Engine)
endif()

# ---- Test helper: send WM_CLOSE to X-Engine window ----
add_executable(send_close tests/send_close.cpp)
if(WIN32)
    target_link_libraries(send_close PRIVATE user32.lib)
endif()

# ---- Tests ----
option(XENGINE_BUILD_TESTS "Build X-Engine unit tests" ON)
if(XENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
