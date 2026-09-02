#pragma once

#include <cstdint>

namespace xe {

enum class Key : uint8_t {
    Unknown = 0,
    Escape,
    W, A, S, D,
    Space,
    Enter,
    Tab,
    Left, Right, Up, Down,
    F1, F2, F3, F4,
    LeftShift, RightShift,
    LeftCtrl, RightCtrl,
    Count
};

}  // namespace xe
