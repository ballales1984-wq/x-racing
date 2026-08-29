#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace p0::race {

// ---------------------------------------------------------------------------
// Race type presets
// ---------------------------------------------------------------------------
enum class RaceType : uint8_t {
  SPRINT = 0,
  ENDURANCE,
  CUSTOM
};

// ---------------------------------------------------------------------------
// Tire compound types
// ---------------------------------------------------------------------------
enum class TireCompound : uint8_t {
  SOFT = 0,
  MEDIUM,
  HARD,
  WET,
  INTERMEDIATE,
  COUNT
};

inline const char* tire_compound_name(TireCompound compound) {
  switch (compound) {
    case TireCompound::SOFT:        return "Soft";
    case TireCompound::MEDIUM:      return "Medium";
    case TireCompound::HARD:        return "Hard";
    case TireCompound::WET:         return "Wet";
    case TireCompound::INTERMEDIATE:return "Intermediate";
    default:                        return "Unknown";
  }
}

// ---------------------------------------------------------------------------
// Violation / penalty types
// ---------------------------------------------------------------------------
enum class ViolationType : uint8_t {
  NONE = 0,
  PIT_SPEED_EXCEEDED,
  TRACK_LIMITS,
  JUMP_START,
  UNSAFE_RELEASE,
  COUNT
};

enum class PenaltyType : uint8_t {
  NONE = 0,
  DRIVE_THROUGH,
  STOP_AND_GO,
  TIME_PENALTY,
  DISQUALIFICATION
};

// ---------------------------------------------------------------------------
// Pit entry policy
// ---------------------------------------------------------------------------
enum class PitEntryPolicy : uint8_t {
  FREE_FOR_ALL = 0,
  FIFO,
  PRIORITY
};

// ---------------------------------------------------------------------------
// Speed detection mode
// ---------------------------------------------------------------------------
enum class SpeedDetectionMode : uint8_t {
  AVERAGE_SPEED = 0,
  INSTANTANEOUS,
  MAX_SPEED
};

// ---------------------------------------------------------------------------
// Car status in race
// ---------------------------------------------------------------------------
enum class CarRaceStatus : uint8_t {
  IN_GARAGE = 0,
  SPAWNED,
  FORMATION_LAP,
  GRID_POSITION,
  RACING,
  PIT_APPROACH,
  PIT_ENTRY,
  PIT_LANE,
  BOX_APPROACH,
  BOX_STOP,
  BOX_SERVICE,
  BOX_RELEASE,
  PIT_EXIT,
  TRACK_REENTRY,
  FINISHED,
  DNF,
  PENALTY_SERVED
};

// ---------------------------------------------------------------------------
// Pit stop state
// ---------------------------------------------------------------------------
enum class PitStopState : uint8_t {
  NONE = 0,
  REQUESTED,
  APPROACHING_PIT_LANE,
  ENTERING_PIT_LANE,
  PIT_LANE_NAVIGATION,
  BOX_ASSIGNED,
  ALIGNING_BOX,
  STOPPED_AT_BOX,
  SERVICING,
  RELEASE_AUTHORIZED,
  EXITING_BOX,
  PIT_EXIT_NAVIGATION,
  TRACK_REENTRY,
  COMPLETE,
  ABANDONED
};

// ---------------------------------------------------------------------------
// Box state
// ---------------------------------------------------------------------------
enum class BoxState : uint8_t {
  FREE = 0,
  OCCUPIED,
  SERVICING,
  RELEASING
};

// ---------------------------------------------------------------------------
// Race session state
// ---------------------------------------------------------------------------
enum class RaceSessionState : uint8_t {
  PREGAME = 0,
  FORMATION,
  GRID,
  GREEN_FLAG,
  GREEN_FLAG_RUNNING,
  SAFETY_CAR,
  CHECKERED_FLAG,
  POST_RACE
};

// ---------------------------------------------------------------------------
// Service type flags
// ---------------------------------------------------------------------------
struct ServiceFlags {
  bool refueling : 1 = false;
  bool tire_change : 1 = false;
  bool repair : 1 = false;
};

// ---------------------------------------------------------------------------
// Car fuel and tire data
// ---------------------------------------------------------------------------
struct CarFuelState {
  double current_fuel_l = 0.0;
  double fuel_capacity_l = 0.0;
  double consumption_per_lap_l = 0.0;
  double consumption_per_m_l = 0.0;
};

struct CarTireState {
  TireCompound current_compound = TireCompound::MEDIUM;
  int current_stint_laps = 0;
  double wear_percent = 0.0;
};

// ---------------------------------------------------------------------------
// Validation result
// ---------------------------------------------------------------------------
enum class ValidationSeverity : uint8_t {
  Error = 0,
  WARNING,
  INFO
};

struct ValidationIssue {
  ValidationSeverity severity = ValidationSeverity::INFO;
  std::string code;
  std::string message;
  std::string affected_component;
};

// ---------------------------------------------------------------------------
// RaceDefinition — complete configuration for a single race
// ---------------------------------------------------------------------------
struct RaceDefinition {
  // --- Mandatory ---
  std::string race_id;
  std::string track_id;
  RaceType race_type = RaceType::SPRINT;
  int max_cars = 20;
  int grid_slots = 20;
  int laps = 15;
  double race_distance_m = 0.0;          // 0 = use laps
  int formation_lap = 1;

  // --- Pit policy ---
  ServiceFlags services;
  PitEntryPolicy pit_entry_policy = PitEntryPolicy::FREE_FOR_ALL;

  // --- Optional constraints ---
  int pit_max_cars = 4;
  int pit_max_stopped = 2;
  int pit_min_stops = 0;
  double speed_tolerance_m_s = 1.39;
  PenaltyType violation_penalty = PenaltyType::DRIVE_THROUGH;
  double pit_min_time_s = 2.5;
  double pit_max_time_s = 120.0;

  // --- Service parameters ---
  double refuel_rate_l_s = 2.0;
  double tire_change_time_s = 3.0;
  double repair_rate_hp_s = 5.0;
  double pit_queue_limit = 8;

  // --- Fuel model ---
  double fuel_capacity_l = 110.0;
  double fuel_consumption_base_l_m = 0.018;
  double fuel_consumption_slope_l_m2 = 0.0;

  // Derived helpers
  bool is_pit_required() const { return pit_min_stops > 0; }
  bool uses_laps() const { return race_distance_m <= 0.0; }
};

// ---------------------------------------------------------------------------
// CarAssignment — per-car race assignment
// ---------------------------------------------------------------------------
struct CarAssignment {
  int car_id = 0;
  int team_id = 0;
  int grid_slot = 0;
  int pit_box_id = -1;
  double start_fuel_l = 0.0;
  TireCompound start_tire = TireCompound::MEDIUM;
  int car_number = 0;
  std::string driver_name;
};

// ---------------------------------------------------------------------------
// TeamDefinition — team metadata
// ---------------------------------------------------------------------------
struct TeamDefinition {
  int team_id = 0;
  std::string team_name;
  int car_count = 1;
  std::vector<int> car_ids;
  int primary_box_id = -1;
  int secondary_box_id = -1;   // for 2-car teams sharing
};

}
