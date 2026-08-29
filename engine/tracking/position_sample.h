#pragma once

#include "common.h"

// Project 0 — tracking / GPS abstraction
// Namespace: p0::tracking
namespace p0::tracking {

// Raw GPS/position sample from any provider.
// Coordinate frame: WGS84 latitude/longitude + altitude.
struct PositionSample {
  double timestamp = 0.0;           // s, absolute timestamp (sim or real time)
  double latitude = 0.0;            // deg, WGS84 latitude
  double longitude = 0.0;           // deg, WGS84 longitude
  double altitude = 0.0;            // m, above WGS84 ellipsoid

  double speed = 0.0;               // m/s, ground speed
  double heading = 0.0;             // rad, true heading (0 = north, clockwise)

  double horizontal_accuracy = 0.0; // m, 1-sigma horizontal error
  double vertical_accuracy = 0.0;   // m, 1-sigma vertical error

  bool valid = false;               // true if the fix is usable
};

}  // namespace p0::tracking
