#include "platform/win32/win32_hud.h"
#include <cassert>

namespace xe {

HudOverlay::~HudOverlay() {
    Shutdown();
}

bool HudOverlay::Initialize(HWND hwnd) {
    hwnd_ = hwnd;
    RECT rc;
    GetClientRect(hwnd_, &rc);
    width_ = rc.right - rc.left;
    height_ = rc.bottom - rc.top;

    HDC screen = GetDC(hwnd_);
    mem_dc_ = CreateCompatibleDC(screen);
    mem_bmp_ = CreateCompatibleBitmap(screen, width_, height_);
    old_bmp_ = static_cast<HBITMAP>(SelectObject(mem_dc_, mem_bmp_));
    ReleaseDC(hwnd_, screen);

    font_ = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        ANTIALIASED_QUALITY, FF_SWISS, L"Consolas");
    old_font_ = static_cast<HFONT>(SelectObject(mem_dc_, font_));
    SetBkMode(mem_dc_, TRANSPARENT);
    SetTextColor(mem_dc_, RGB(255, 255, 255));

    return true;
}

void HudOverlay::Shutdown() {
    if (mem_dc_) {
        if (old_bmp_) SelectObject(mem_dc_, old_bmp_);
        if (old_font_) SelectObject(mem_dc_, old_font_);
        if (font_) DeleteObject(font_);
        if (mem_bmp_) DeleteObject(mem_bmp_);
        DeleteDC(mem_dc_);
        mem_dc_ = nullptr;
        mem_bmp_ = nullptr;
        old_bmp_ = nullptr;
        font_ = nullptr;
        old_font_ = nullptr;
    }
}

void HudOverlay::Resize(int width, int height) {
    if (width == width_ && height == height_) return;
    width_ = width;
    height_ = height;
    if (mem_dc_) {
        if (mem_bmp_) DeleteObject(mem_bmp_);
        HDC screen = GetDC(hwnd_);
        mem_bmp_ = CreateCompatibleBitmap(screen, width_, height_);
        old_bmp_ = static_cast<HBITMAP>(SelectObject(mem_dc_, mem_bmp_));
        ReleaseDC(hwnd_, screen);
    }
}

void HudOverlay::BeginDraw() {
    if (mem_dc_) {
        HBRUSH bg = CreateSolidBrush(RGB(0, 0, 0));
        RECT rc{ 0, 0, width_, height_ };
        FillRect(mem_dc_, &rc, bg);
        DeleteObject(bg);
    }
}

void HudOverlay::EndDraw() {
    if (mem_dc_ && hwnd_) {
        HDC screen = GetDC(hwnd_);
        BitBlt(screen, 0, 0, width_, height_, mem_dc_, 0, 0, SRCCOPY);
        ReleaseDC(hwnd_, screen);
    }
}

void HudOverlay::DrawText(int x, int y, const std::wstring& text, COLORREF color) {
    if (!mem_dc_) return;
    SetTextColor(mem_dc_, color);
    TextOutW(mem_dc_, x, y, text.c_str(), static_cast<int>(text.size()));
}

void HudOverlay::DrawLine(int x0, int y0, int x1, int y1, COLORREF color) {
    if (!mem_dc_) return;
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HPEN old = static_cast<HPEN>(SelectObject(mem_dc_, pen));
    MoveToEx(mem_dc_, x0, y0, nullptr);
    LineTo(mem_dc_, x1, y1);
    SelectObject(mem_dc_, old);
    DeleteObject(pen);
}

}  // namespace xe