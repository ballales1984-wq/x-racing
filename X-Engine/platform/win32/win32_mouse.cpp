#include "platform/win32/win32_mouse.h"
#include <algorithm>

namespace xe {

void Win32Mouse::Update() {
    for (int i = 0; i < 3; ++i) {
        state_.prev_buttons[i] = state_.buttons[i];
    }
    state_.dx = 0;
    state_.dy = 0;
    state_.wheel = 0;
}

void Win32Mouse::OnMouseMove(int x, int y) {
    if (has_last_pos_) {
        state_.dx = x - last_x_;
        state_.dy = y - last_y_;
    } else {
        state_.dx = 0;
        state_.dy = 0;
        has_last_pos_ = true;
    }
    state_.x = x;
    state_.y = y;
    last_x_ = x;
    last_y_ = y;
}

void Win32Mouse::OnMouseDown(MouseButton button) {
    int idx = static_cast<int>(button);
    if (idx >= 0 && idx < 3) state_.buttons[idx] = true;
}

void Win32Mouse::OnMouseUp(MouseButton button) {
    int idx = static_cast<int>(button);
    if (idx >= 0 && idx < 3) state_.buttons[idx] = false;
}

void Win32Mouse::OnMouseWheel(int delta) {
    state_.wheel += delta;
}

}  // namespace xe