# GoogleTest: reuse parent repo's copy if already configured, otherwise add it
if(NOT TARGET GTest::gtest)
    set(GOOGLETEST_VERSION 1.16.0)
    set(XENGINE_GOOGLETEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/googletest/googletest"
        CACHE PATH "Path to GoogleTest root")
    if(EXISTS "${XENGINE_GOOGLETEST_DIR}/CMakeLists.txt")
        add_subdirectory(${XENGINE_GOOGLETEST_DIR} ${CMAKE_BINARY_DIR}/xengine-googletest-build)
    else()
        message(WARNING "GoogleTest not found — skipping X-Engine tests")
        return()
    endif()
endif()

add_executable(xe_tests
    test_main.cpp
    logger_test.cpp
    clock_test.cpp
    application_test.cpp
    renderer_test.cpp
)

target_link_libraries(xe_tests PRIVATE xe_core GTest::gtest)

include(GoogleTest)
gtest_discover_tests(xe_tests)
