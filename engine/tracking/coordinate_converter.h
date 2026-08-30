#pragma once

#include "common.h"
#include "tracking/position_sample.h"

// Project 0 — tracking / local-to-geographic conversion
// Namespace: p0::tracking
namespace p0::tracking {

// Reference point that anchors a local (simulation) frame to the Earth.
// Local positions are expressed in meters relative to this origin.
struct GeographicOrigin {
  double latitude = 45.0;    // deg, WGS84 latitude of the local frame origin
  double longitude = 11.0;   // deg, WGS84 longitude of the local frame origin
  double altitude = 0.0;     // m, altitude of the local frame origin
};

// Converts between a local simulation frame (meters) and a geographic
// WGS84 frame (latitude/longitude/altitude). Independent of Track, VehicleState
// and any rendering engine — it only needs a GeographicOrigin.
//
// The conversion uses an equirectangular approximation around the origin,
// which is accurate for the scale of a race circuit.
class CoordinateConverter {
 public:
  static constexpr GeographicOrigin kDefaultOrigin{45.0, 11.0, 0.0};

  explicit CoordinateConverter(GeographicOrigin origin = kDefaultOrigin);

  void set_origin(GeographicOrigin origin) { origin_ = origin; }
  const GeographicOrigin& origin() const { return origin_; }

  // Local simulation coordinates (meters) -> geographic PositionSample.
  // heading and speed are forwarded unchanged (radians / m/s).
  PositionSample local_to_geographic(const Vec2& local_position,
                                     double heading,
                                     double speed,
                                     double timestamp) const;

  // Inverse of local_to_geographic: geographic -> local meters.
  Vec2 geographic_to_local(double latitude, double longitude) const;

 private:
  GeographicOrigin origin_;
};

}  // namespace p0::tracking
