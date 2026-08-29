#pragma once

#include "common.h"

// Project 0 — tracking / track-relative coordinate
// Namespace: p0::tracking
namespace p0::tracking {

// Track-relative position produced by TrackMapper.
// Independent of any geodetic frame or rendering engine.
struct TrackPosition {
  double s = 0.0;           // m, cumulative distance along centerline from start
  double lateral = 0.0;     // m, signed offset from centerline (+left, -right)

  double heading = 0.0;     // rad, track heading at nearest centerline point
  double curvature = 0.0;   // 1/m, signed curvature at nearest point
  double track_width = 0.0; // m, usable track width at nearest point
  double banking = 0.0;     // rad, banking angle at nearest point

  int segment_index = -1;   // index into track sample array
  bool on_track = false;    // true if mapped within track boundaries
  double distance_to_centerline = 0.0; // m, Euclidean distance to centerline
};

}  // namespace p0::tracking
