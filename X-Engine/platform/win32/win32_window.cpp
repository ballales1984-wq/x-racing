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

void* Win32Window::GetNativeDC() const {
    return GetDC(hwnd_);
}

void Win32Window::SetCursorCapture(bool captured) {
    if (!hwnd_) return;
    if (captured == cursor_captured_) return;
    cursor_captured_ = captured;
    if (captured) {
        RECT rc;
        GetClientRect(hwnd_, &rc);
        POINT center{ (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
        ClientToScreen(hwnd_, &center);
        SetCursorPos(center.x, center.y);
        ShowCursor(FALSE);
        // Capture mouse so we get exclusive deltas
        SetCapture(hwnd_);
        // Clip cursor to client area
        RECT clip = rc;
        POINT tl{ clip.left, clip.top };
        POINT br{ clip.right, clip.bottom };
        ClientToScreen(hwnd_, &tl);
        ClientToScreen(hwnd_, &br);
        clip.left = tl.x;   clip.top = tl.y;
        clip.right = br.x;  clip.bottom = br.y;
        ClipCursor(&clip);
    } else {
        ShowCursor(TRUE);
        ClipCursor(nullptr);
        ReleaseCapture();
    }
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

        case WM_MOUSEMOVE:
            if (mouse_) {
                mouse_->OnMouseMove(LOWORD(lparam), HIWORD(lparam));
            }
            return 0;

        case WM_LBUTTONDOWN: if (mouse_) mouse_->OnMouseDown(MouseButton::Left);   return 0;
        case WM_LBUTTONUP:   if (mouse_) mouse_->OnMouseUp(MouseButton::Left);     return 0;
        case WM_RBUTTONDOWN: if (mouse_) mouse_->OnMouseDown(MouseButton::Right);  return 0;
        case WM_RBUTTONUP:   if (mouse_) mouse_->OnMouseUp(MouseButton::Right);    return 0;
        case WM_MBUTTONDOWN: if (mouse_) mouse_->OnMouseDown(MouseButton::Middle); return 0;
        case WM_MBUTTONUP:   if (mouse_) mouse_->OnMouseUp(MouseButton::Middle);   return 0;
        case WM_MOUSEWHEEL:  if (mouse_) mouse_->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wparam)); return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }
    return DefWindowProc(hwnd_, msg, wparam, lparam);
}

}  // namespace xe
