#include "validation.h"
#include <cmath>
#include <algorithm>
#include <unordered_set>

namespace p0::track {

std::string ValidationEngine::severity_name(p0::race::ValidationSeverity s) {
  switch (s) {
    case p0::race::ValidationSeverity::ERROR:   return "ERROR";
    case p0::race::ValidationSeverity::WARNING: return "WARNING";
    case p0::race::ValidationSeverity::INFO:    return "INFO";
    default:                          return "UNKNOWN";
  }
}

ValidationEngine::ValidationEngine(const TrackData& track,
                                   const RaceDefinition& race,
                                   const std::vector<CarAssignment>& assignments)
    : track_(track), race_(race), assignments_(assignments) {}

std::vector<p0::race::ValidationIssue> ValidationEngine::validate_all() {
  std::vector<p0::race::ValidationIssue> issues;
  auto append = [&](std::vector<p0::race::ValidationIssue> src) {
    issues.insert(issues.end(), std::make_move_iterator(src.begin()),
                               std::make_move_iterator(src.end()));
  };
  append(validate_geometry());
  append(validate_direction());
  append(validate_grid());
  append(validate_pit_lane());
  append(validate_race());
  append(validate_assignments());
  return issues;
}

std::vector<p0::race::ValidationIssue> ValidationEngine::validate_geometry() const {
  std::vector<p0::race::ValidationIssue> issues;

  if (track_.waypoints.size() < 4) {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "GEO_001",
                      "Track needs at least 4 waypoints",
                      "TrackGeometry"});
  }

  if (track_.length_m < 100.0) {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "GEO_002",
                      "Track length too short (< 100 m)",
                      "TrackGeometry"});
  }

  if (track_.racing_line.empty()) {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "GEO_003",
                      "Racing line is empty",
                      "TrackGeometry"});
  }

  if (track_.start_finish.transform.position == Vec2::Zero() &&
      track_.start_finish.transform.forward == Vec2::Zero()) {
    issues.push_back({p0::race::ValidationSeverity::WARNING, "GEO_004",
                      "Start/finish line not explicitly defined",
                      "TrackGeometry"});
  }

  return issues;
}

std::vector<p0::race::ValidationIssue> ValidationEngine::validate_direction() const {
  std::vector<p0::race::ValidationIssue> issues;

  if (track_.direction != "CLOCKWISE" && track_.direction != "COUNTER_CLOCKWISE") {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "DIR_001",
                      "Track direction must be CLOCKWISE or COUNTER_CLOCKWISE",
                      "TrackDirection"});
  }

  if (track_.waypoints.size() >= 2) {
    Vec2 w0_fwd = track_.waypoints[0].transform.forward;
    Vec2 sf_fwd = track_.start_finish.transform.forward;
    double dot = w0_fwd.dot(sf_fwd);
    if (dot < 0.0) {
      issues.push_back({p0::race::ValidationSeverity::ERROR, "DIR_002",
                        "Start/finish forward direction contradicts first waypoint",
                        "TrackDirection"});
    }
  }

  return issues;
}

std::vector<p0::race::ValidationIssue> ValidationEngine::validate_grid() const {
  std::vector<p0::race::ValidationIssue> issues;

  if (race_.max_cars > track_.grid.max_slots) {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "GRID_001",
                      "Race max_cars exceeds grid max_slots: " +
                      std::to_string(race_.max_cars) + " > " +
                      std::to_string(track_.grid.max_slots),
                      "GridSystem"});
  }

  if (static_cast<int>(track_.grid.slots.size()) < race_.grid_slots) {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "GRID_002",
                      "Defined grid slots (" +
                      std::to_string(track_.grid.slots.size()) +
                      ") less than required (" +
                      std::to_string(race_.grid_slots) + ")",
                      "GridSystem"});
  }

  for (size_t i = 0; i < track_.grid.slots.size(); ++i) {
    for (size_t j = i + 1; j < track_.grid.slots.size(); ++j) {
      const auto& a = track_.grid.slots[i];
      const auto& b = track_.grid.slots[j];
      double dist = (a.transform.position - b.transform.position).norm();
      if (dist < 3.5) {
        issues.push_back({p0::race::ValidationSeverity::ERROR, "GRID_003",
                          "Grid slots " + std::to_string(a.slot_id) +
                          " and " + std::to_string(b.slot_id) +
                          " overlap (distance: " + std::to_string(dist) + " m)",
                          "GridSystem"});
      }
    }
  }

  return issues;
}

std::vector<p0::race::ValidationIssue> ValidationEngine::validate_pit_lane() const {
  std::vector<p0::race::ValidationIssue> issues;

  if (track_.pit_lane.path.empty()) {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "PIT_001",
                      "Pit lane path is empty",
                      "PitLane"});
  }

  if (track_.pit_lane.boxes.empty()) {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "PIT_002",
                      "No pit boxes defined",
                      "PitLane"});
  }

  if (track_.pit_lane.speed_zone.speed_limit_m_s <= 0.0) {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "PIT_003",
                      "Speed limit must be > 0",
                      "PitLane"});
  }

  for (const auto& box : track_.pit_lane.boxes) {
    bool reachable = false;
    for (const auto& p : track_.pit_lane.path) {
      if ((p.transform.position - box.position.position).norm() < 20.0) {
        reachable = true;
        break;
      }
    }
    if (!reachable) {
      issues.push_back({p0::race::ValidationSeverity::ERROR, "PIT_004",
                        "Pit box " + std::to_string(box.box_id) +
                        " is not reachable from pit lane path",
                        "PitLane"});
    }
  }

  for (size_t i = 0; i < track_.pit_lane.boxes.size(); ++i) {
    for (size_t j = i + 1; j < track_.pit_lane.boxes.size(); ++j) {
      const auto& a = track_.pit_lane.boxes[i];
      const auto& b = track_.pit_lane.boxes[j];
      double dist = (a.position.position - b.position.position).norm();
      if (dist < 4.0) {
        issues.push_back({p0::race::ValidationSeverity::ERROR, "PIT_005",
                          "Pit boxes overlap (distance: " +
                          std::to_string(dist) + " m)",
                          "PitLane"});
      }
    }
  }

  return issues;
}

std::vector<p0::race::ValidationIssue> ValidationEngine::validate_race() const {
  std::vector<p0::race::ValidationIssue> issues;

  if (race_.max_cars > race_.grid_slots) {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "RACE_001",
                      "max_cars exceeds grid_slots",
                      "RaceRules"});
  }

  if (race_.laps < 1 && race_.race_distance_m <= 0.0) {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "RACE_002",
                      "Race must have at least 1 lap or positive distance",
                      "RaceRules"});
  }

  if (race_.pit_min_stops > race_.laps && race_.laps > 0) {
    issues.push_back({p0::race::ValidationSeverity::ERROR, "RACE_003",
                      "pit_min_stops exceeds total laps",
                      "RaceRules"});
  }

  if (race_.services.refueling &&
      race_.pit_min_stops > race_.laps) {
    issues.push_back({p0::race::ValidationSeverity::WARNING, "RACE_004",
                      "Refueling required but min_stops > laps",
                      "RaceRules"});
  }

  return issues;
}

std::vector<p0::race::ValidationIssue> ValidationEngine::validate_assignments() const {
  std::vector<p0::race::ValidationIssue> issues;

  std::unordered_set<int> used_slots;
  std::unordered_set<int> used_boxes;

  for (const auto& a : assignments_) {
    if (used_slots.count(a.grid_slot)) {
      issues.push_back({p0::race::ValidationSeverity::ERROR, "ASGN_001",
                        "Duplicate grid slot " + std::to_string(a.grid_slot) +
                        " for car " + std::to_string(a.car_id),
                        "CarAssignments"});
    }
    used_slots.insert(a.grid_slot);

    if (a.pit_box_id >= 0) {
      if (used_boxes.count(a.pit_box_id)) {
        issues.push_back({p0::race::ValidationSeverity::ERROR, "ASGN_002",
                          "Duplicate pit box " + std::to_string(a.pit_box_id) +
                          " for car " + std::to_string(a.car_id),
                          "CarAssignments"});
      }
      used_boxes.insert(a.pit_box_id);
    }

    if (a.start_fuel_l > race_.fuel_capacity_l) {
      issues.push_back({p0::race::ValidationSeverity::ERROR, "ASGN_003",
                        "Start fuel exceeds capacity for car " +
                        std::to_string(a.car_id),
                        "CarAssignments"});
    }
  }

  return issues;
}

}
