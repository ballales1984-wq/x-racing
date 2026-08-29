#pragma once

#include "../../common.h"
#include "../input_manager.h"
#include "../input.h"
#define NOMINMAX
#include <windows.h>

// Project 0 — Windows input manager using GetAsyncKeyState
// Namespace: p0::input
namespace p0::input {

class WindowsInputManager : public InputManager {
 public:
  WindowsInputManager() = default;
  ~WindowsInputManager() override = default;

  InputState poll() override;
  bool is_key_down(int key_code) override;

 private:
  static constexpr int kThrottleKey = 'W';
  static constexpr int kBrakeKey = 'S';
  static constexpr int kLeftKey = 'A';
  static constexpr int kRightKey = 'D';
  static constexpr int kUpshiftKey = VK_SHIFT;
  static constexpr int kDownshiftKey = VK_CONTROL;
  static constexpr int kResetKey = 'R';
  static constexpr int kEscapeKey = VK_ESCAPE;
  static constexpr int kBoxKey = 'B';
};

}
