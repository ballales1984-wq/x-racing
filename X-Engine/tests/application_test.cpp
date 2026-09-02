#include <gtest/gtest.h>
#include "core/application.h"
#include "core/clock.h"
#include "rendering/renderer.h"
#include "tests/fake_platform.h"

using namespace xe;

TEST(ApplicationTest, CreateReturnsTrue) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    Application app(std::move(window), std::move(input));

    EXPECT_TRUE(app.Create("Test App", 800, 600));
    app.Shutdown();
}

TEST(ApplicationTest, WindowSizeAfterCreate) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    Application app(std::move(window), std::move(input));

    ASSERT_TRUE(app.Create("Test", 1024, 768));

    int w, h;
    app.GetWindow().GetSize(w, h);
    EXPECT_EQ(w, 1024);
    EXPECT_EQ(h, 768);

    app.Shutdown();
}

TEST(ApplicationTest, RunLoopPollsEvents) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    test::FakeWindow* window_ptr = window.get();
    window->auto_close_after = 5;
    Application app(std::move(window), std::move(input));

    ASSERT_TRUE(app.Create("Test", 800, 600));
    app.Run();

    EXPECT_EQ(window_ptr->poll_count, 5);

    app.Shutdown();
}

TEST(ApplicationTest, ShouldCloseBreaksLoop) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    test::FakeWindow* window_ptr = window.get();
    window->auto_close_after = 3;
    Application app(std::move(window), std::move(input));

    ASSERT_TRUE(app.Create("Test", 800, 600));
    app.Run();

    EXPECT_GE(window_ptr->poll_count, 3);

    app.Shutdown();
}

TEST(ApplicationTest, EscapeKeyBreaksLoop) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    test::FakeInput* input_ptr = input.get();
    input->SimulateKeyPressOnFrame(Key::Escape, 3);
    Application app(std::move(window), std::move(input));

    ASSERT_TRUE(app.Create("Test", 800, 600));
    app.Run();

    EXPECT_EQ(input_ptr->update_count, 3);

    app.Shutdown();
}

TEST(ApplicationTest, ShutdownIsCallable) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    window->auto_close_after = 1;
    Application app(std::move(window), std::move(input));

    ASSERT_TRUE(app.Create("Test", 800, 600));
    app.Run();
    app.Shutdown();

    SUCCEED();
}

TEST(ApplicationTest, OnUpdateReceivedDeltaTime) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    window->auto_close_after = 3;
    Application app(std::move(window), std::move(input));

    ASSERT_TRUE(app.Create("Test", 800, 600));
    app.Run();

    EXPECT_GT(app.GetClock().GetDeltaTime(), 0.0f);
    EXPECT_GT(app.GetClock().GetTotalTime(), 0.0f);
    EXPECT_GE(app.GetClock().GetFrameCount(), 1);

    app.Shutdown();
}

TEST(ApplicationTest, NoShutdownWithoutCreate) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    test::FakeWindow* window_ptr = window.get();
    Application app(std::move(window), std::move(input));

    EXPECT_FALSE(window_ptr->create_called);

    app.Shutdown();

    SUCCEED();
}
