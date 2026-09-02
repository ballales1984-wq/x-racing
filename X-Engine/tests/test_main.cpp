#include <gtest/gtest.h>
#include "core/logger.h"

int main(int argc, char** argv) {
    xe::Logger::Init();
    testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    xe::Logger::Shutdown();
    return result;
}
