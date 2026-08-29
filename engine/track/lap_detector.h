// X-Racing — runtime lap detector
// Author: alessio
#pragma once

#include "common.h"

// Project 0 — track subsystem
// Namespace: p0::track
namespace p0::track {

// LapDetector: runtime detection of completed laps by watching the car's
// arc-length progress along the track centerline.
//
// A lap is counted only when the car crosses the start/finish line in the
// forward (correct) direction. Crossing the line backwards (e.g. while
// reversing) is detected and used to undo a previously counted lap instead
// of double-counting. This makes lap counting robust even when the car
// moves backwards, which the naive "distance >= track length" check cannot do.
class LapDetector {
 public:
  explicit LapDetector(double track_length, int total_laps = 0)
      : track_length_(track_length > kEpsilon ? track_length : kEpsilon),
        total_laps_(total_laps) {}

  // Feed the current distance along the track (already wrapped into [0, L)).
  // Returns true exactly on a valid forward crossing of the start/finish line.
  bool update(double distance) {
    if (!initialized_) {
      prev_distance_ = distance;
      initialized_ = true;
      return false;
    }

    const double delta = distance - prev_distance_;
    bool crossed_forward = false;

    if (delta < -track_length_ * 0.5) {
      crossed_forward = true;
      ++completed_laps_;
    } else if (delta > track_length_ * 0.5) {
      completed_laps_ = (completed_laps_ > 0) ? completed_laps_ - 1 : 0;
    }

    prev_distance_ = distance;
    return crossed_forward;
  }

  // Re-arm the detector (e.g. on race reset) without changing total_laps.
  void reset(double start_distance = 0.0) {
    completed_laps_ = 0;
    prev_distance_ = start_distance;
    initialized_ = false;
  }

  void set_track_length(double track_length) {
    track_length_ = track_length > kEpsilon ? track_length : kEpsilon;
  }

  int completed_laps() const { return completed_laps_; }
  int total_laps() const { return total_laps_; }
  void set_total_laps(int total_laps) { total_laps_ = total_laps; }
  bool finished() const {
    return total_laps_ > 0 && completed_laps_ >= total_laps_;
  }

 private:
  double track_length_;
  int total_laps_;
  int completed_laps_ = 0;
  double prev_distance_ = 0.0;
  bool initialized_ = false;
};

}  // namespace p0::track
