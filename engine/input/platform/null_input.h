#pragma once

#include "../../common.h"
#include "../input_manager.h"
#include "../input.h"

// Project 0 — null input manager for tests and headless operation
// Namespace: p0::input
namespace p0::input {

class NullInputManager : public InputManager {
 public:
  NullInputManager() = default;
  ~NullInputManager() override = default;

  InputState poll() override { return InputState{}; }
  bool is_key_down(int /*key_code*/) override { return false; }
};

}
