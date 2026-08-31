#pragma once

#include "race_config.h"
#include "track_data.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace p0::track {

// ---------------------------------------------------------------------------
// Pit Service Unit types
// ---------------------------------------------------------------------------
enum class PSUType : uint8_t {
  JACK = 0,
  COMPRESSOR,
  FUEL_DISPENSER,
  TIRE_CHANGER,
  COUNT
};

inline const char* psu_type_name(PSUType type) {
  switch (type) {
    case PSUType::JACK:          return "Jack";
    case PSUType::COMPRESSOR:    return "Compressor";
    case PSUType::FUEL_DISPENSER:return "FuelDispenser";
    case PSUType::TIRE_CHANGER:  return "TireChanger";
    default:                     return "Unknown";
  }
}

// ---------------------------------------------------------------------------
// Pit Service Unit state
// ---------------------------------------------------------------------------
enum class PSUState : uint8_t {
  IDLE = 0,
  READY,
  ACTIVE,
  FAULT,
  MAINTENANCE
};

inline const char* psu_state_name(PSUState state) {
  switch (state) {
    case PSUState::IDLE:         return "IDLE";
    case PSUState::READY:        return "READY";
    case PSUState::ACTIVE:       return "ACTIVE";
    case PSUState::FAULT:        return "FAULT";
    case PSUState::MAINTENANCE:  return "MAINTENANCE";
    default:                     return "UNKNOWN";
  }
}

// ---------------------------------------------------------------------------
// PitServiceUnit — single equipment unit in a box
// ---------------------------------------------------------------------------
struct PitServiceUnit {
  int unit_id = 0;
  PSUType type = PSUType::JACK;
  PSUState state = PSUState::IDLE;
  int assigned_box_id = -1;
  int assigned_car_id = -1;
  double power_rating_w = 0.0;
  double active_since = 0.0;
  double total_usage_time_s = 0.0;
  int fault_count = 0;
  std::string fault_message;
};

// ---------------------------------------------------------------------------
// BoxPSUConfig — per-box PSU allocation
// ---------------------------------------------------------------------------
struct BoxPSUConfig {
  int box_id = -1;
  std::vector<int> psu_ids;
  bool has_jack = false;
  bool has_compressor = false;
  bool has_fuel_dispenser = false;
  bool has_tire_changer = false;
  double max_power_w = 0.0;
};

// ---------------------------------------------------------------------------
// PitServiceUnitManager — manages all PSUs in the pit lane
// ---------------------------------------------------------------------------
class PitServiceUnitManager {
 public:
  explicit PitServiceUnitManager(const PitLaneDefinition& pit_def);

  void initialize_units();
  bool assign_units_to_box(int box_id, const std::vector<PSUType>& required);
  void release_box(int box_id);
  bool activate_units_for_service(int box_id, int car_id, double timestamp);
  void deactivate_units(int box_id, double timestamp);
  bool is_service_ready(int box_id) const;
  double estimated_service_time(int box_id, bool refuel, bool tires, bool repair) const;
  std::vector<PitServiceUnit> units_for_box(int box_id) const;
  const PitServiceUnit* find_unit(int unit_id) const;

  void update_faults(double timestamp);
  void set_maintenance(int unit_id, bool in_maintenance);

  const std::vector<PitServiceUnit>& units() const { return units_; }
  std::vector<BoxPSUConfig> box_configs() const { return box_configs_; }

 private:
  PitLaneDefinition def_;
  std::vector<PitServiceUnit> units_;
  std::vector<BoxPSUConfig> box_configs_;
  std::unordered_map<int, int> box_to_primary_unit_;
  std::unordered_map<int, std::vector<int>> box_to_units_;

  int next_unit_id_ = 0;
  void create_units_for_boxes();
  BoxPSUConfig build_box_config(int box_index) const;
};

}
