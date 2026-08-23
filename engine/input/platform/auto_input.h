#pragma once

#include "../../common.h"
#include "../input_manager.h"
#include "../input.h"

// Project 0 — automatic input manager for recorded/replay drives
// Namespace: p0::input
namespace p0::input {

class AutoInputManager : public InputManager {
 public:
  AutoInputManager() = default;
  ~AutoInputManager() override = default;

  InputState poll() override;
  bool is_key_down(int /*key_code*/) override { return false; }

 private:
  double elapsed_ = 0.0;
};

}
