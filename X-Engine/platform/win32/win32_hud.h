#pragma once

#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>

namespace xe {

class Console;

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

    // Filled rectangle (for console background panel)
    void DrawRect(int x, int y, int w, int h, COLORREF fill);

    // ASCII text (for console)
    void DrawTextA(int x, int y, const std::string& text, COLORREF color = RGB(255,255,255));

    // Render a console panel: input line + recent output (top N lines).
    void DrawConsole(Console& console, int screen_w, int screen_h);

    int Width() const { return width_; }
    int Height() const { return height_; }

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