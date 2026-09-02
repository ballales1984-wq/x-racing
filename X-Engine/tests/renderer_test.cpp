#include <gtest/gtest.h>
#include "core/application.h"
#include "core/clock.h"
#include "rendering/renderer.h"
#include "tests/fake_platform.h"

using namespace xe;

namespace xe::test {

class FakeRenderer : public Renderer {
public:
    bool Initialize(uintptr_t window_handle) override {
        handle_ = window_handle;
        initialize_called = true;
        return should_succeed;
    }
    void BeginFrame() override { begin_count++; }
    void EndFrame() override { end_count++; }
    void Resize(uint32_t w, uint32_t h) override {
        resize_calls.emplace_back(w, h);
    }

    uintptr_t handle_ = 0;
    bool initialize_called = false;
    bool should_succeed = true;
    int begin_count = 0;
    int end_count = 0;
    std::vector<std::pair<uint32_t, uint32_t>> resize_calls;
};

}  // namespace xe::test

TEST(RendererTest, ApplicationPassesNullptrByDefault) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    Application app(std::move(window), std::move(input));

    ASSERT_TRUE(app.Create("Test", 800, 600));
    app.Shutdown();
}

TEST(RendererTest, ApplicationInitializesRenderer) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    auto renderer = std::make_unique<test::FakeRenderer>();
    test::FakeRenderer* renderer_ptr = renderer.get();

    Application app(std::move(window), std::move(input), std::move(renderer));
    ASSERT_TRUE(app.Create("Test", 800, 600));

    EXPECT_TRUE(renderer_ptr->initialize_called);
    app.Shutdown();
}

TEST(RendererTest, RendererNotInitializedWhenNoWindow) {
    auto renderer = std::make_unique<test::FakeRenderer>();
    test::FakeRenderer* renderer_ptr = renderer.get();

    Application app(nullptr, nullptr, std::move(renderer));
    EXPECT_FALSE(app.Create("Test", 800, 600));
    EXPECT_FALSE(renderer_ptr->initialize_called);
}

TEST(RendererTest, ResizeCallbackForwardedToRenderer) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    auto renderer = std::make_unique<test::FakeRenderer>();
    test::FakeWindow* window_ptr = window.get();
    test::FakeRenderer* renderer_ptr = renderer.get();

    Application app(std::move(window), std::move(input), std::move(renderer));
    ASSERT_TRUE(app.Create("Test", 800, 600));

    window_ptr->SimulateResize(1280, 720);

    ASSERT_EQ(renderer_ptr->resize_calls.size(), 1u);
    EXPECT_EQ(renderer_ptr->resize_calls[0].first, 1280u);
    EXPECT_EQ(renderer_ptr->resize_calls[0].second, 720u);

    app.Shutdown();
}

TEST(RendererTest, BeginEndFrameCalledInRunLoop) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    auto renderer = std::make_unique<test::FakeRenderer>();
    test::FakeWindow* window_ptr = window.get();
    test::FakeRenderer* renderer_ptr = renderer.get();
    window_ptr->auto_close_after = 5;

    Application app(std::move(window), std::move(input), std::move(renderer));
    ASSERT_TRUE(app.Create("Test", 800, 600));
    app.Run();

    EXPECT_EQ(renderer_ptr->begin_count, renderer_ptr->end_count);
    EXPECT_GE(renderer_ptr->begin_count, 1);
    app.Shutdown();
}

TEST(RendererTest, RendererInitFailureFailsAppCreate) {
    auto window = std::make_unique<test::FakeWindow>();
    auto input = std::make_unique<test::FakeInput>();
    auto renderer = std::make_unique<test::FakeRenderer>();
    renderer->should_succeed = false;

    Application app(std::move(window), std::move(input), std::move(renderer));
    EXPECT_FALSE(app.Create("Test", 800, 600));
}
