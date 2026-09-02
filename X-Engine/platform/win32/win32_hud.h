#pragma once

#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>

namespace xe {

class HudOverlay {
public:
    HudOverlay() = default;
    ~HudOverlay();

    HudOverlay(const HudOverlay&) = delete;
    HudOverlay& operator=(const HudOverlay&) = delete;

    bool Initialize(HWND hwnd);
    void Shutdown();

    void Resize(int width, int height);
    void BeginDraw();
    void EndDraw();

    void DrawText(int x, int y, const std::wstring& text, COLORREF color = RGB(255,255,255));
    void DrawLine(int x0, int y0, int x1, int y1, COLORREF color = RGB(255,255,255));

private:
    HWND hwnd_ = nullptr;
    HDC mem_dc_ = nullptr;
    HBITMAP mem_bmp_ = nullptr;
    HBITMAP old_bmp_ = nullptr;
    HFONT font_ = nullptr;
    HFONT old_font_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

}  // namespace xe