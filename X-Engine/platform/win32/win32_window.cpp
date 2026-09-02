#include "platform/win32/win32_window.h"
#include "core/logger.h"

namespace xe {

Win32Window::~Win32Window() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

bool Win32Window::Create(const std::string& title, int width, int height) {
    hinstance_ = GetModuleHandle(nullptr);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hinstance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"XEngineWindowClass";

    if (!RegisterClassEx(&wc)) {
        XE_LOG_ERROR("Failed to register window class");
        return false;
    }

    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    std::wstring wtitle(title.begin(), title.end());

    hwnd_ = CreateWindowEx(
        0,
        L"XEngineWindowClass",
        wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, hinstance_, nullptr);

    if (!hwnd_) {
        XE_LOG_ERROR("Failed to create window");
        return false;
    }

    ShowWindow(hwnd_, SW_SHOWNORMAL);
    UpdateWindow(hwnd_);

    return true;
}

void Win32Window::PollEvents() {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Win32Window::GetSize(int& width, int& height) const {
    RECT rect;
    GetClientRect(hwnd_, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}

uintptr_t Win32Window::GetNativeHandle() const {
    return reinterpret_cast<uintptr_t>(hwnd_);
}

LRESULT Win32Window::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        auto* self = reinterpret_cast<Win32Window*>(
            reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    auto* self = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (self) {
        return self->HandleMessage(msg, wparam, lparam);
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT Win32Window::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CLOSE:
            should_close_ = true;
            return 0;

        case WM_SIZE:
            if (resize_callback_ && wparam != SIZE_MINIMIZED) {
                int w = LOWORD(lparam);
                int h = HIWORD(lparam);
                resize_callback_(w, h);
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }
    return DefWindowProc(hwnd_, msg, wparam, lparam);
}

}  // namespace xe
