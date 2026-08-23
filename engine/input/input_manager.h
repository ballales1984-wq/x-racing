#pragma once

#include "common.h"
#include "input.h"

// Project 0 — abstract input manager
// Namespace: p0::input
namespace p0::input {

// Abstract interface for platform-specific input polling.
// Implementations: WindowsInputManager, NullInputManager, SDLInputManager, etc.
class InputManager {
 public:
  virtual ~InputManager() = default;

  // Poll current input state and return normalized InputState.
  // This should be called once per frame.
  virtual InputState poll() = 0;

  // Check if a specific key is currently pressed.
  // Key codes are platform-dependent; use the platform-specific subclass.
  virtual bool is_key_down(int key_code) = 0;
};

}
