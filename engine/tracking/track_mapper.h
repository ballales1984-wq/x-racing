#pragma once

#include "common.h"
#include "tracking/position_sample.h"
#include "tracking/track_position.h"
#include "track/track.h"

// Project 0 — tracking / track coordinate mapping
// Namespace: p0::tracking
namespace p0::tracking {

// Maps a GPS PositionSample onto track-relative TrackPosition coordinates.
// Depends on a parametric p0::track::Track but does not modify it.
class TrackMapper {
 public:
  explicit TrackMapper(const p0::track::Track& track);

  // Convert geodetic sample to track-relative coordinates.
  // Returns a valid TrackPosition; on_track is false if mapping fails.
  TrackPosition map(const PositionSample& sample) const;

 private:
  const p0::track::Track* track_;
};

}  // namespace p0::tracking
