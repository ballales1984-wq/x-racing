#pragma once

#include "keycode.h"

namespace xe {

class Input {
public:
    virtual ~Input() = default;

    virtual void Update() = 0;
    virtual bool IsKeyDown(Key key) const = 0;
    virtual bool IsKeyPressed(Key key) const = 0;
};

}  // namespace xe
