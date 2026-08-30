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
  struct LocalOrigin {
    double latitude = 0.0;
    double longitude = 0.0;
  };

  explicit TrackMapper(const p0::track::Track& track, LocalOrigin origin = {});

  void set_local_origin(LocalOrigin origin);
  const LocalOrigin& local_origin() const { return origin_; }

  TrackPosition map(const PositionSample& sample) const;

 private:
  Vec2 geodetic_to_local(double latitude, double longitude) const;

  const p0::track::Track* track_;
  LocalOrigin origin_;
};

}  // namespace p0::tracking
