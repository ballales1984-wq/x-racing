#include "windows_input.h"
#include <windows.h>

namespace p0::input {

// Poll the keyboard via GetAsyncKeyState and build a normalized InputState.
// Supports both WASD and arrow keys for driving, plus Shift/Ctrl for gear
// changes, R for reset, and B for box lane entry/exit.
// The high-order bit (0x8000) indicates the key is currently held down.
InputState WindowsInputManager::poll() {
  InputState input;

  // Primary driving controls (WASD + arrows).
  if (GetAsyncKeyState(kThrottleKey) & 0x8000) input.throttle = 1.0;
  if (GetAsyncKeyState(kBrakeKey) & 0x8000) input.brake = 1.0;
  if (GetAsyncKeyState(kLeftKey) & 0x8000) input.steering = -1.0;
  if (GetAsyncKeyState(kRightKey) & 0x8000) input.steering = 1.0;
  if (GetAsyncKeyState(VK_UP) & 0x8000) input.throttle = 1.0;
  if (GetAsyncKeyState(VK_DOWN) & 0x8000) input.brake = 1.0;
  if (GetAsyncKeyState(VK_LEFT) & 0x8000) input.steering = -1.0;
  if (GetAsyncKeyState(VK_RIGHT) & 0x8000) input.steering = 1.0;

  // Auxiliary controls (edge-triggered boolean flags).
  if (GetAsyncKeyState(kUpshiftKey) & 0x8000) input.upshift = true;
  if (GetAsyncKeyState(kDownshiftKey) & 0x8000) input.downshift = true;
  if (GetAsyncKeyState(kResetKey) & 0x8000) input.reset = true;
  if (GetAsyncKeyState(kBoxKey) & 0x8000) input.enter_exit_box = true;
  if (GetAsyncKeyState(kReverseKey) & 0x8000) input.reverse = true;

  // Normalize near-zero steering to full deflection (digital input cleanup).
  if (std::abs(input.steering) < 0.1 && std::abs(input.steering) > 0.0) {
    input.steering = input.steering > 0 ? 1.0 : -1.0;
  }

  return input;
}

// Returns true if the given virtual key code is currently held down.
// A convenience wrapper around GetAsyncKeyState for external polling.
bool WindowsInputManager::is_key_down(int key_code) {
  return (GetAsyncKeyState(key_code) & 0x8000) != 0;
}

}
