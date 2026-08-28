#pragma once

#include "common.h"

// Project 0 — parametric track model
// Namespace: p0::track
namespace p0::track {

// Pavement / ground surface type with characteristic friction coefficient.
// Coefficients are dimensionless multipliers applied to the base tire mu.
enum class SurfaceType : uint8_t {
  Asphalt = 0,      // 1.00 — dry racing asphalt
  WetAsphalt,       // 0.70 — wet asphalt
  OldAsphalt,       // 0.85 — worn / dirty asphalt
  Kerb,             // 0.65 — curbing (lower grip)
  Grass,            // 0.30 — short grass
  Gravel,           // 0.40 — gravel trap
  Dirt,             // 0.50 — dirt / clay
  Sand,             // 0.25 — sand / loose surface
  Count
};

inline double friction_for_surface(SurfaceType type) {
  switch (type) {
    case SurfaceType::Asphalt:      return 1.00;
    case SurfaceType::WetAsphalt:   return 0.70;
    case SurfaceType::OldAsphalt:   return 0.85;
    case SurfaceType::Kerb:         return 0.65;
    case SurfaceType::Grass:        return 0.30;
    case SurfaceType::Gravel:       return 0.40;
    case SurfaceType::Dirt:         return 0.50;
    case SurfaceType::Sand:         return 0.25;
    default:                        return 1.00;
  }
}

// Single sample point along the track centerline
struct TrackPoint {
  Vec2 position;                         // m, world position
  Vec2 tangent;                          // unit vector, forward direction
  Vec2 normal;                           // unit vector, leftward direction
  double curvature = 0.0;                // 1/m, signed curvature
  double width = 12.0;                   // m, track width at this point
  double banking = 0.0;                  // rad, banking angle
  double friction = 1.0;                 // [-], local friction modifier
  double distance = 0.0;                 // m, cumulative distance from start
  SurfaceType surface_type = SurfaceType::Asphalt;

  bool has_box_lane = false;             // true if a box/pit lane exists here
  double box_lane_width = 3.5;           // m, width of the box lane
};

// Parameters for track generation
struct TrackParams {
  double total_length = 5000.0;          // m, closed-loop length
  double default_width = 12.0;           // m
  double default_friction = 1.0;         // [-]
  SurfaceType default_surface = SurfaceType::Asphalt;
};

// Track layout variant.
enum class TrackType : uint8_t {
  Default = 0,    // original oval-ish circuit with box lane on first straight
  PitCircuit,     // road course with dedicated pit lane and pit boxes
  CustomCircuit  // custom road course with clear direction and box lane
};

// Parametric closed-loop track.
// The track is built from a sequence of precomputed points.
// Querying at any distance returns interpolated geometry.
class Track {
 public:
  explicit Track(const TrackParams& params = {});
  explicit Track(TrackType type, const TrackParams& params = {});
  ~Track() = default;

  TrackType track_type() const { return type_; }
  const std::vector<double>& pit_box_positions() const { return pit_box_positions_; }

  // Get interpolated track data at distance d (wraps around loop)
  TrackPoint at(double distance) const;
  // Starting position and heading for a new lap
  Vec2 get_start_position() const;
  double get_start_heading() const;
  double length() const { return total_length_; }

  bool has_box_lane_at(double distance) const;
  double box_lane_width_at(double distance) const;

  // Surface / coefficient API
  SurfaceType surface_type_at(double distance) const;
  void set_surface_at(double distance, SurfaceType type);
  static double friction_for_surface(SurfaceType type) {
    return ::p0::track::friction_for_surface(type);
  }

  private:
   void build_default_track();
   void build_pit_track();
   void build_custom_track();
   TrackPoint interpolate(double distance, int i0, int i1, double frac) const;
   void find_adjacent_points(double distance, int& i0, int& i1, double& frac) const;

  std::vector<TrackPoint> points_;
  std::vector<double> pit_box_positions_;
  double total_length_ = 0.0;
  double default_width_ = 12.0;
  double default_friction_ = 1.0;
  SurfaceType default_surface_ = SurfaceType::Asphalt;
  TrackType type_ = TrackType::Default;
};

}
