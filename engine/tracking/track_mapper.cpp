#include "tracking/track_mapper.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace p0::tracking {

TrackMapper::TrackMapper(const p0::track::Track& track, LocalOrigin origin)
    : track_(&track), origin_(origin) {}

void TrackMapper::set_local_origin(LocalOrigin origin) { origin_ = origin; }

Vec2 TrackMapper::geodetic_to_local(double latitude, double longitude) const {
  const double lat_rad = origin_.latitude * kDegToRad;
  const double meters_per_deg_lat = 111132.92;
  const double meters_per_deg_lon = 111412.84 * std::cos(lat_rad);

  const double dx = (longitude - origin_.longitude) * meters_per_deg_lon;
  const double dy = (latitude - origin_.latitude) * meters_per_deg_lat;

  return Vec2(dx, dy);
}

TrackPosition TrackMapper::map(const PositionSample& sample) const {
  TrackPosition result{};
  result.on_track = false;

  if (!sample.valid || !track_) return result;

  const Vec2 gps_pos = geodetic_to_local(sample.latitude, sample.longitude);

  const double track_len = track_->length();
  if (track_len <= 0.0) return result;

  const int count = track_->sample_count();
  if (count <= 0) return result;

  double best_dist = std::numeric_limits<double>::infinity();
  int best_index = 0;

  for (int i = 0; i < count; ++i) {
    const p0::track::TrackPoint& tp = track_->sample_at(i);
    const double d = (gps_pos - tp.position).norm();
    if (d < best_dist) {
      best_dist = d;
      best_index = i;
    }
  }

  const p0::track::TrackPoint nearest = track_->at(track_->sample_at(best_index).distance);
  const Vec2 to_point = gps_pos - nearest.position;
  const double lateral = to_point.dot(nearest.normal);

  result.s = nearest.distance;
  result.lateral = lateral;
  result.heading = std::atan2(nearest.tangent.y(), nearest.tangent.x());
  result.curvature = nearest.curvature;
  result.track_width = nearest.width;
  result.banking = nearest.banking;
  result.segment_index = best_index;
  result.distance_to_centerline = best_dist;

  result.on_track = (best_dist <= nearest.width * 0.5 + 5.0);
  return result;
}

}  // namespace p0::tracking
