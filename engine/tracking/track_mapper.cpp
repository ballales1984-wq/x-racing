#include "tracking/track_mapper.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace p0::tracking {

TrackMapper::TrackMapper(const p0::track::Track& track) : track_(&track) {}

TrackPosition TrackMapper::map(const PositionSample& sample) const {
  TrackPosition result{};
  result.on_track = false;

  if (!sample.valid || !track_) return result;

  const Vec2 gps_pos(sample.longitude, sample.latitude);

  const double track_len = track_->length();
  if (track_len <= 0.0) return result;

  const int samples = 200;
  double best_dist = std::numeric_limits<double>::infinity();
  double best_s = 0.0;

  for (int i = 0; i < samples; ++i) {
    const double s = (static_cast<double>(i) / samples) * track_len;
    const p0::track::TrackPoint tp = track_->at(s);
    const double d = (gps_pos - tp.position).norm();
    if (d < best_dist) {
      best_dist = d;
      best_s = s;
    }
  }

  const p0::track::TrackPoint nearest = track_->at(best_s);
  const Vec2 to_point = gps_pos - nearest.position;
  const double lateral = to_point.dot(nearest.normal);

  result.s = best_s;
  result.lateral = lateral;
  result.heading = std::atan2(nearest.tangent.y(), nearest.tangent.x());
  result.curvature = nearest.curvature;
  result.track_width = nearest.width;
  result.banking = nearest.banking;
  result.segment_index = -1;
  result.distance_to_centerline = best_dist;

  result.on_track = (best_dist <= nearest.width * 0.5 + 5.0);
  return result;
}

}  // namespace p0::tracking
