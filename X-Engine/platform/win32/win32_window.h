#pragma once

#define NOMINMAX
#include <windows.h>
#include "../window.h"
#include "../mouse.h"
#include <functional>
#include <string>

namespace xe {

class Win32Window : public Window {
public:
    Win32Window() = default;
    ~Win32Window() override;

    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;

bool Create(const std::string& title, int width, int height) override;
    void PollEvents() override;
    bool ShouldClose() const override { return should_close_; }
    void GetSize(int& width, int& height) const override;
    uintptr_t GetNativeHandle() const override;
    HWND GetHWND() const { return hwnd_; }
    void SetResizeCallback(ResizeCallback callback) override { resize_callback_ = std::move(callback); }
    void SetMouse(Mouse* mouse) { mouse_ = mouse; }
    void* GetNativeDC() const override;
    void SetCursorCapture(bool captured);
    bool IsCursorCaptured() const { return cursor_captured_; }

    // ASCII character received via WM_CHAR (excluding control chars).
    using CharCallback = std::function<void(char)>;
    void SetCharCallback(CharCallback cb) { char_callback_ = std::move(cb); }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

    HWND hwnd_ = nullptr;
    HINSTANCE hinstance_ = nullptr;
    bool should_close_ = false;
    bool cursor_captured_ = false;
    ResizeCallback resize_callback_;
    CharCallback char_callback_;
    Mouse* mouse_ = nullptr;
};

}  // namespace xe
