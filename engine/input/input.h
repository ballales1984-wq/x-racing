#pragma once

#include "common.h"

// Project 0 — input abstraction
// Namespace: p0::input
namespace p0::input {

// Driver input for a single simulation step
// All continuous values are normalized:
//   throttle, brake: [0, 1]
//   steering: [-1, +1] (left/right)
struct InputState {
  double throttle = 0.0;
  double brake = 0.0;
  double steering = 0.0;
  bool upshift = false;
  bool downshift = false;
  bool reset = false;
};

}
