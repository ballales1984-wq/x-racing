#include "windows_input.h"
#include <windows.h>

namespace p0::input {

InputState WindowsInputManager::poll() {
  InputState input;

  if (GetAsyncKeyState(kThrottleKey) & 0x8000) input.throttle = 1.0;
  if (GetAsyncKeyState(kBrakeKey) & 0x8000) input.brake = 1.0;
  if (GetAsyncKeyState(kLeftKey) & 0x8000) input.steering = -1.0;
  if (GetAsyncKeyState(kRightKey) & 0x8000) input.steering = 1.0;
  if (GetAsyncKeyState(VK_UP) & 0x8000) input.throttle = 1.0;
  if (GetAsyncKeyState(VK_DOWN) & 0x8000) input.brake = 1.0;
  if (GetAsyncKeyState(VK_LEFT) & 0x8000) input.steering = -1.0;
  if (GetAsyncKeyState(VK_RIGHT) & 0x8000) input.steering = 1.0;
  if (GetAsyncKeyState(kUpshiftKey) & 0x8000) input.upshift = true;
  if (GetAsyncKeyState(kDownshiftKey) & 0x8000) input.downshift = true;
  if (GetAsyncKeyState(kResetKey) & 0x8000) input.reset = true;

  if (std::abs(input.steering) < 0.1 && std::abs(input.steering) > 0.0) {
    input.steering = input.steering > 0 ? 1.0 : -1.0;
  }

  return input;
}

bool WindowsInputManager::is_key_down(int key_code) {
  return (GetAsyncKeyState(key_code) & 0x8000) != 0;
}

}
