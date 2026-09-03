#pragma once

#include <cstdint>

namespace xe {

enum class MouseButton : uint8_t {
    Left = 0,
    Right,
    Middle,
    Count,
};

struct MouseState {
    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = 0;
    int wheel = 0;
    bool buttons[3] = { false, false, false };
    bool prev_buttons[3] = { false, false, false };

    bool IsDown(MouseButton b) const {
        return buttons[static_cast<int>(b)];
    }

    bool WasPressed(MouseButton b) const {
        int i = static_cast<int>(b);
        return buttons[i] && !prev_buttons[i];
    }

    bool WasReleased(MouseButton b) const {
        int i = static_cast<int>(b);
        return !buttons[i] && prev_buttons[i];
    }
};

class Mouse {
public:
    virtual ~Mouse() = default;

    virtual void Update() = 0;

    // Window message hooks (Win32 backend forwards them; tests can call directly)
    virtual void OnMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) {}
    virtual void OnMouseDown([[maybe_unused]] MouseButton /*button*/) {}
    virtual void OnMouseUp([[maybe_unused]] MouseButton /*button*/) {}
    virtual void OnMouseWheel([[maybe_unused]] int /*delta*/) {}

    const MouseState& GetState() const { return state_; }

protected:
    MouseState state_;
};

}  // namespace xe