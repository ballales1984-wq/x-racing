#pragma once

#define NOMINMAX
#include <windows.h>
#include "../mouse.h"

namespace xe {

class Win32Mouse : public Mouse {
public:
    Win32Mouse() = default;
    ~Win32Mouse() override = default;

    Win32Mouse(const Win32Mouse&) = delete;
    Win32Mouse& operator=(const Win32Mouse&) = delete;

    void Update() override;
    void OnMouseMove(int x, int y);
    void OnMouseDown(MouseButton button);
    void OnMouseUp(MouseButton button);
    void OnMouseWheel(int delta);

private:
    int last_x_ = 0;
    int last_y_ = 0;
    bool has_last_pos_ = false;
};

}  // namespace xe