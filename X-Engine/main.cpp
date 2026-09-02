#include "core/logger.h"
#include "core/application.h"
#include "rendering/dx12/dx12_renderer.h"
#include "platform/win32/win32_window.h"
#include "platform/win32/win32_input.h"

int main() {
    xe::Logger::Init();
    XE_LOG_INFO("X-Engine V0.2");

    auto window = std::make_unique<xe::Win32Window>();
    auto input = std::make_unique<xe::Win32Input>();
    auto renderer = std::make_unique<xe::DX12Renderer>();

    xe::Application app(std::move(window), std::move(input), std::move(renderer));
    if (!app.Create("X-Engine V0.2", 1280, 720)) {
        XE_LOG_ERROR("Failed to initialize engine");
        xe::Logger::Shutdown();
        return -1;
    }

    app.Run();
    app.Shutdown();

    XE_LOG_INFO("Done");
    xe::Logger::Shutdown();
    return 0;
}
