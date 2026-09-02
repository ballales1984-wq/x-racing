#pragma once

#define NOMINMAX
#include <windows.h>
#include "../input.h"
#include <array>

namespace xe {

class Win32Input : public Input {
public:
    Win32Input() = default;
    ~Win32Input() override = default;

    Win32Input(const Win32Input&) = delete;
    Win32Input& operator=(const Win32Input&) = delete;

    void Update() override;
    bool IsKeyDown(Key key) const override;
    bool IsKeyPressed(Key key) const override;

private:
    static int ToNativeKey(Key key);

    std::array<BYTE, 256> current_keys_{};
    std::array<BYTE, 256> previous_keys_{};
};

}  // namespace xe
