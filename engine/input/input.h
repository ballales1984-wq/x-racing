#pragma once

#include "common.h"

namespace p0::input {

struct InputState {
  double throttle = 0.0;
  double brake = 0.0;
  double steering = 0.0;
  bool upshift = false;
  bool downshift = false;
  bool reset = false;
};

}
