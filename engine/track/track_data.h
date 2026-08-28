#pragma once

#include "common.h"
#include "race_config.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace p0::track {

using p0::Vec2;
using p0::kEpsilon;
using p0::race::SpeedDetectionMode;
using p0::race::ViolationType;
using p0::race::PenaltyType;
using p0::race::BoxState;

// ---------------------------------------------------------------------------
// 2D position + orientation
// ---------------------------------------------------------------------------
struct Transform2D {
  Vec2 position{0.0, 0.0};
  Vec2 forward{1.0, 0.0};
};

inline Transform2D make_transform(const Vec2& pos, const Vec2& fwd) {
  Transform2D t;
  t.position = pos;
  t.forward = fwd.normalized();
  return t;
}

// ---------------------------------------------------------------------------
// Grid slot definition
// ---------------------------------------------------------------------------
struct GridSlot {
  int slot_id = 0;
  Transform2D transform;
  double width = 4.0;
  double depth = 10.0;
};

enum class GridLayout : uint8_t {
  SINGLE_COLUMN = 0,
  TWO_COLUMN,
  CUSTOM
};

struct GridDefinition {
  std::vector<GridSlot> slots;
  int max_slots = 30;
  GridLayout layout = GridLayout::SINGLE_COLUMN;
  double row_spacing = 8.0;
  double column_spacing = 6.0;

  int slot_count() const { return static_cast<int>(slots.size()); }
  bool has_slot(int id) const {
    for (const auto& s : slots) if (s.slot_id == id) return true;
    return false;
  }
};

// ---------------------------------------------------------------------------
// Waypoint
// ---------------------------------------------------------------------------
struct Waypoint {
  int id = 0;
  Transform2D transform;
  double width = 15.0;
  double trigger_radius = 3.0;
};

// ---------------------------------------------------------------------------
// Racing line sample
// ---------------------------------------------------------------------------
struct RacingLineSample {
  Transform2D transform;
  double speed_m_s = 0.0;
  double throttle = 0.0;
  double brake = 0.0;
  double gear = 1.0;
};

// ---------------------------------------------------------------------------
// Checkpoint
// ---------------------------------------------------------------------------
struct Checkpoint {
  int id = 0;
  Transform2D transform;
  double width = 20.0;
  bool is_sector_gate = false;
  int sector_index = 0;
};

// ---------------------------------------------------------------------------
// Start / Finish line
// ---------------------------------------------------------------------------
struct StartFinishLine {
  Transform2D transform;
  double width = 15.0;
};

// ---------------------------------------------------------------------------
// Pit lane entry / exit
// ---------------------------------------------------------------------------
struct PitEntryPoint {
  Transform2D transform;
  double width = 5.0;
};

struct PitExitPoint {
  Transform2D transform;
  double width = 5.0;
};

// ---------------------------------------------------------------------------
// Speed detection zone (average speed: two timing lines)
// ---------------------------------------------------------------------------
struct SpeedDetectionZone {
  Transform2D start_line;
  Transform2D end_line;
  double speed_limit_m_s = 16.67;
  double tolerance_m_s = 1.39;
  SpeedDetectionMode detection_mode = SpeedDetectionMode::AVERAGE_SPEED;
  ViolationType violation_type = ViolationType::PIT_SPEED_EXCEEDED;
  PenaltyType penalty = PenaltyType::DRIVE_THROUGH;

  double effective_limit_m_s() const { return speed_limit_m_s + tolerance_m_s; }
};

// ---------------------------------------------------------------------------
// Pit lane path segment
// ---------------------------------------------------------------------------
struct PitLanePathPoint {
  Transform2D transform;
  double width = 4.0;
};

// ---------------------------------------------------------------------------
// Merge zone
// ---------------------------------------------------------------------------
struct MergeZone {
  Transform2D start;
  Transform2D end;
  double length() const { return (end.position - start.position).norm(); }
};

// ---------------------------------------------------------------------------
// Pit box
// ---------------------------------------------------------------------------
struct PitBox {
  int box_id = 0;
  int team_id = 0;
  Transform2D position;
  Transform2D service_position;
  Vec2 entry_direction;
  Vec2 exit_direction;
  double width = 4.0;
  double depth = 6.0;
  BoxState state = BoxState::FREE;
  int assigned_car_id = -1;
  double occupied_since = 0.0;
};

// ---------------------------------------------------------------------------
// Complete Pit Lane definition
// ---------------------------------------------------------------------------
struct PitLaneDefinition {
  PitEntryPoint entry;
  SpeedDetectionZone speed_zone;
  std::vector<PitLanePathPoint> path;
  std::vector<PitBox> boxes;
  PitExitPoint exit;
  MergeZone merge_zone;
  double speed_limit_m_s = 16.67;
  double pit_lane_length_m = 0.0;

  int box_count() const { return static_cast<int>(boxes.size()); }
  bool is_valid() const { return !path.empty() && box_count() > 0; }
};

// ---------------------------------------------------------------------------
// Complete Track Data (engine-agnostic, serializable)
// ---------------------------------------------------------------------------
struct TrackData {
  // Metadata
  std::string track_id;
  std::string track_name;
  double length_m = 0.0;
  double width_m = 15.0;
  double surface_grip = 1.0;
  std::string direction = "COUNTER_CLOCKWISE";

  // Geometry
  StartFinishLine start_finish;
  GridDefinition grid;
  std::vector<Waypoint> waypoints;
  std::vector<RacingLineSample> racing_line;
  std::vector<Checkpoint> checkpoints;

  // Pit
  PitLaneDefinition pit_lane;

  // Safety zones (runoff, gravel traps, etc.)
  std::vector<Transform2D> safety_zones;

  // Methods
  bool is_valid() const { return !track_id.empty() && length_m > 0.0; }
  int max_grid_slots() const { return grid.max_slots; }
};

}
