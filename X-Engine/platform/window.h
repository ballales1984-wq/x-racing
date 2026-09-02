#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace xe {

using ResizeCallback = std::function<void(int, int)>;

class Window {
public:
    virtual ~Window() = default;

    virtual bool Create(const std::string& title, int width, int height) = 0;
    virtual void PollEvents() = 0;
    virtual bool ShouldClose() const = 0;
    virtual void GetSize(int& width, int& height) const = 0;
    virtual uintptr_t GetNativeHandle() const = 0;
    virtual void SetResizeCallback(ResizeCallback callback) = 0;
    virtual void* GetNativeDC() const = 0;
    virtual void SetCursorCapture([[maybe_unused]] bool captured) {}
    virtual bool IsCursorCaptured() const { return false; }
};

}  // namespace xe
