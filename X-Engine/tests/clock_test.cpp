#include <gtest/gtest.h>
#include "core/clock.h"
#include <thread>
#include <chrono>

TEST(ClockTest, StartsWithReset) {
    xe::Clock clock;
    clock.Reset();
    EXPECT_EQ(clock.GetFrameCount(), 0);
    EXPECT_FLOAT_EQ(clock.GetTotalTime(), 0.0f);
    EXPECT_FLOAT_EQ(clock.GetDeltaTime(), 0.0f);
}

TEST(ClockTest, TickIncrementsFrameCount) {
    xe::Clock clock;
    clock.Reset();
    for (int i = 0; i < 10; i++) {
        clock.Tick();
    }
    EXPECT_EQ(clock.GetFrameCount(), 10);
    EXPECT_GT(clock.GetTotalTime(), 0.0f);
}

TEST(ClockTest, DeltaTimeIsPositive) {
    xe::Clock clock;
    clock.Reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    float dt = clock.Tick();
    EXPECT_GT(dt, 0.0f);
    EXPECT_GT(clock.GetFPS(), 0.0f);
}

TEST(ClockTest, DeltaTimeClamped) {
    xe::Clock clock;
    clock.Reset();
    // Simulate a long frame by sleeping > 100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    float dt = clock.Tick();
    EXPECT_LE(dt, 0.1f);
}

TEST(ClockTest, TotalTimeAccumulates) {
    xe::Clock clock;
    clock.Reset();
    float t0 = clock.GetTotalTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    clock.Tick();
    float t1 = clock.GetTotalTime();
    EXPECT_GT(t1, t0);
}
