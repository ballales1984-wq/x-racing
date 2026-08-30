#pragma once

#include "common.h"

// Project 0 — tracking / surface cell model
// Namespace: p0::tracking
namespace p0::tracking {

// Per-cell surface state for dynamic grip modeling.
struct SurfaceCell {
  double temperature = 20.0;   // deg C
  double rubber = 0.0;         // rubber deposition [0, 1+] normalized
  double moisture = 0.0;       // water film [0, 1]
  double grip = 1.0;           // computed grip multiplier
  double base_grip = 1.0;      // underlying surface grip
  double last_update = 0.0;    // s, timestamp of last update
};

}  // namespace p0::tracking
