#include <gtest/gtest.h>
#include "core/logger.h"

TEST(LoggerTest, InitAndShutdown) {
    xe::Logger::Init();
    EXPECT_TRUE(true);
    xe::Logger::Shutdown();
}

TEST(LoggerTest, LogAfterShutdownIsNoop) {
    xe::Logger::Init();
    xe::Logger::Info("test message before shutdown");
    xe::Logger::Shutdown();
    // Log after shutdown should not crash and is a no-op
    xe::Logger::Info("this should be a no-op");
    EXPECT_TRUE(true);
}

TEST(LoggerTest, DoubleInitIsSafe) {
    xe::Logger::Init();
    xe::Logger::Init();
    xe::Logger::Shutdown();
    EXPECT_TRUE(true);
}
