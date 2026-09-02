#include "core/application.h"
#include "core/logger.h"
#include "core/clock.h"
#include "rendering/renderer.h"
#include "platform/window.h"
#include "platform/input.h"
#include "platform/keycode.h"

namespace xe {

Application::Application(std::unique_ptr<Window> window, std::unique_ptr<Input> input,
                         std::unique_ptr<Renderer> renderer)
    : window_(std::move(window))
    , input_(std::move(input))
    , renderer_(std::move(renderer)) {
}

Application::~Application() {
    if (running_) {
        Shutdown();
    }
}

bool Application::Create(const std::string& title, int width, int height) {
    if (!window_) {
        XE_LOG_ERROR("No window provided to Application");
        return false;
    }

    if (!window_->Create(title, width, height)) {
        XE_LOG_ERROR("Failed to create window");
        return false;
    }

    if (!input_) {
        XE_LOG_ERROR("No input provided to Application");
        return false;
    }

    clock_ = std::make_unique<Clock>();
    clock_->Reset();

    if (renderer_) {
        if (!renderer_->Initialize(window_->GetNativeHandle())) {
            XE_LOG_ERROR("Failed to initialize renderer");
            return false;
        }
        window_->SetResizeCallback([this](int w, int h) {
            if (renderer_) {
                renderer_->Resize(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
            }
        });
    }

    running_ = true;
    XE_LOG_INFO("X-Engine V0.2 initialized");
    return true;
}

void Application::Run() {
    XE_LOG_INFO("Entering main loop");

    while (running_) {
        window_->PollEvents();

        float dt = clock_->Tick();
        input_->Update();

        if (window_->ShouldClose()) {
            running_ = false;
            break;
        }

        OnUpdate(dt);

        if (renderer_) {
            renderer_->BeginFrame(clock_->GetTotalTime());
            OnRender(dt);
            renderer_->EndFrame();
        }
    }

    XE_LOG_INFO("Exited main loop");
}

void Application::Shutdown() {
    OnShutdown();

    XE_LOG_INFO("Shutting down X-Engine");

    running_ = false;
    window_.reset();
    input_.reset();
    clock_.reset();
}

Window& Application::GetWindow() const {
    return *window_;
}

Input& Application::GetInput() const {
    return *input_;
}

Clock& Application::GetClock() const {
    return *clock_;
}

}  // namespace xe
