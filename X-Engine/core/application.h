#pragma once

#include <memory>
#include <string>

namespace xe {

class Window;
class Input;
class Clock;
class Renderer;

class Application {
public:
    Application(std::unique_ptr<Window> window, std::unique_ptr<Input> input,
                std::unique_ptr<Renderer> renderer = nullptr);
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool Create(const std::string& title = "X-Engine", int width = 1280, int height = 720);
    void Run();
    void Shutdown();

    Window& GetWindow() const;
    Input& GetInput() const;
    Clock& GetClock() const;

protected:
    virtual void OnUpdate(float) {}
    virtual void OnRender(float) {}
    virtual void OnShutdown() {}

private:
    std::unique_ptr<Window> window_;
    std::unique_ptr<Input> input_;
    std::unique_ptr<Clock> clock_;
    std::unique_ptr<Renderer> renderer_;
    bool running_ = false;
};

}  // namespace xe
