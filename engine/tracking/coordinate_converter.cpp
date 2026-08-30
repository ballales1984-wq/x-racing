#include "tracking/coordinate_converter.h"

#include <cmath>

namespace p0::tracking {

CoordinateConverter::CoordinateConverter(GeographicOrigin origin)
    : origin_(origin) {}

PositionSample CoordinateConverter::local_to_geographic(
    const Vec2& local_position, double heading, double speed,
    double timestamp) const {
  const double lat_rad = origin_.latitude * kDegToRad;
  const double meters_per_deg_lat = 111132.92;
  const double meters_per_deg_lon = 111412.84 * std::cos(lat_rad);

  PositionSample sample{};
  sample.timestamp = timestamp;
  sample.latitude = origin_.latitude + local_position.y() / meters_per_deg_lat;
  sample.longitude = origin_.longitude + local_position.x() / meters_per_deg_lon;
  sample.altitude = origin_.altitude;
  sample.heading = heading;
  sample.speed = speed;
  sample.valid = true;
  return sample;
}

Vec2 CoordinateConverter::geographic_to_local(double latitude,
                                              double longitude) const {
  const double lat_rad = origin_.latitude * kDegToRad;
  const double meters_per_deg_lat = 111132.92;
  const double meters_per_deg_lon = 111412.84 * std::cos(lat_rad);

  const double dx = (longitude - origin_.longitude) * meters_per_deg_lon;
  const double dy = (latitude - origin_.latitude) * meters_per_deg_lat;
  return Vec2(dx, dy);
}

}  // namespace p0::tracking
