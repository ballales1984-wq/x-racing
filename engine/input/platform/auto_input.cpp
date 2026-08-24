#include "auto_input.h"

namespace p0::input {

// Scripted driver input used for recorded/replay drives.
// Produces a fixed sequence: full throttle, right curve, left curve,
// braking, then reset and repeat.
InputState AutoInputManager::poll() {
  InputState input;
  const double t = elapsed_;
  elapsed_ += 1.0 / 60.0;

  if (t < 3.0) {
    input.throttle = 1.0;
  } else if (t < 8.0) {
    input.throttle = 1.0;
    input.steering = 1.0;
  } else if (t < 13.0) {
    input.throttle = 1.0;
    input.steering = -1.0;
  } else if (t < 16.0) {
    input.throttle = 0.0;
    input.brake = 1.0;
  } else {
    input.reset = true;
    elapsed_ = 0.0;
  }

  return input;
}

}
