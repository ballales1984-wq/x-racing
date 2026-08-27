#include "auto_input.h"

namespace p0::input {

// Scripted driver input used for recorded/replay drives.
// Produces a fixed time-based sequence:
//   0-3s:   full throttle (straight line acceleration)
//   3-8s:   full throttle + right steering (right-hand curve)
//   8-13s:  full throttle + left steering (left-hand curve)
//   13-16s: braking (deceleration zone)
//   >16s:   reset and restart the sequence
// This loop repeats indefinitely, providing deterministic input for testing.
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
