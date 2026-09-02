#include "pit_service_unit.h"
#include <algorithm>
#include <numeric>

namespace p0::track {

//! @brief Constructs the PSU manager and creates units for all pit boxes.
//! @param pit_def The pit lane definition containing box configurations.
PitServiceUnitManager::PitServiceUnitManager(const PitLaneDefinition& pit_def)
    : def_(pit_def) {
  create_units_for_boxes();
}

//! @brief Creates PSU instances for each pit box based on box configuration.
//!        Each box gets one of each PSU type: JACK, COMPRESSOR, FUEL_DISPENSER, TIRE_CHANGER.
void PitServiceUnitManager::create_units_for_boxes() {
  units_.clear();
  box_configs_.clear();
  box_to_primary_unit_.clear();
  box_to_units_.clear();
  next_unit_id_ = 0;

  for (size_t i = 0; i < def_.boxes.size(); ++i) {
    box_configs_.push_back(build_box_config(static_cast<int>(i)));
    const auto& cfg = box_configs_.back();

    if (cfg.has_jack) {
      units_.push_back(PitServiceUnit{next_unit_id_++, PSUType::JACK, PSUState::IDLE, static_cast<int>(i), -1, 1500.0});
    }
    if (cfg.has_compressor) {
      units_.push_back(PitServiceUnit{next_unit_id_++, PSUType::COMPRESSOR, PSUState::IDLE, static_cast<int>(i), -1, 2200.0});
    }
    if (cfg.has_fuel_dispenser) {
      units_.push_back(PitServiceUnit{next_unit_id_++, PSUType::FUEL_DISPENSER, PSUState::IDLE, static_cast<int>(i), -1, 3000.0});
    }
    if (cfg.has_tire_changer) {
      units_.push_back(PitServiceUnit{next_unit_id_++, PSUType::TIRE_CHANGER, PSUState::IDLE, static_cast<int>(i), -1, 1800.0});
    }

    std::vector<int> ids;
    for (const auto& u : units_) {
      if (u.assigned_box_id == static_cast<int>(i)) ids.push_back(u.unit_id);
    }
    box_to_units_[static_cast<int>(i)] = ids;
    if (!ids.empty()) box_to_primary_unit_[static_cast<int>(i)] = ids[0];
  }
}

//! @brief Builds a default PSU configuration for a pit box.
//! @param box_index The index of the pit box.
//! @return BoxPSUConfig with all PSU types enabled.
BoxPSUConfig PitServiceUnitManager::build_box_config(int box_index) const {
  BoxPSUConfig cfg;
  cfg.box_id = box_index;
  cfg.has_jack = true;
  cfg.has_compressor = true;
  cfg.has_fuel_dispenser = true;
  cfg.has_tire_changer = true;
  cfg.max_power_w = 1500.0 + 2200.0 + 3000.0 + 1800.0;
  return cfg;
}

//! @brief Assigns specific PSU types to a pit box for a service.
//! @param box_id The pit box ID.
//! @param required Vector of required PSU types.
//! @return true if at least one unit was assigned.
bool PitServiceUnitManager::assign_units_to_box(int box_id, const std::vector<PSUType>& required) {
  if (box_id < 0 || static_cast<size_t>(box_id) >= box_configs_.size()) return false;

  release_box(box_id);
  BoxPSUConfig& cfg = box_configs_[box_id];
  for (auto t : required) {
    for (auto& u : units_) {
      if (u.assigned_box_id == box_id && u.type == t) {
        u.state = PSUState::READY;
        cfg.psu_ids.push_back(u.unit_id);
      }
    }
  }
  return !cfg.psu_ids.empty();
}

//! @brief Releases all PSUs from a pit box, resetting them to IDLE state.
//! @param box_id The pit box ID.
void PitServiceUnitManager::release_box(int box_id) {
  if (box_id < 0 || static_cast<size_t>(box_id) >= box_configs_.size()) return;
  BoxPSUConfig& cfg = box_configs_[box_id];
  cfg.psu_ids.clear();
  for (auto& u : units_) {
    if (u.assigned_box_id == box_id) {
      u.state = PSUState::IDLE;
      u.assigned_car_id = -1;
      u.active_since = 0.0;
    }
  }
}

//! @brief Activates all PSUs assigned to a box for service.
//! @param box_id The pit box ID.
//! @param car_id The car being serviced.
//! @param timestamp Current race time in seconds.
//! @return true if all units were successfully activated.
bool PitServiceUnitManager::activate_units_for_service(int box_id, int car_id, double timestamp) {
  if (box_id < 0 || static_cast<size_t>(box_id) >= box_configs_.size()) return false;
  BoxPSUConfig& cfg = box_configs_[box_id];
  if (cfg.psu_ids.empty()) return false;

  for (int uid : cfg.psu_ids) {
    auto* u = find_unit(uid);
    if (!u || u->state != PSUState::READY) return false;
  }

  for (int uid : cfg.psu_ids) {
    auto* u = const_cast<PitServiceUnit*>(find_unit(uid));
    if (u) {
      u->state = PSUState::ACTIVE;
      u->assigned_car_id = car_id;
      u->active_since = timestamp;
    }
  }
  return true;
}

//! @brief Deactivates all PSUs for a box after service completion.
//!        Accumulates total usage time for maintenance tracking.
//! @param box_id The pit box ID.
//! @param timestamp Current race time in seconds.
void PitServiceUnitManager::deactivate_units(int box_id, double timestamp) {
  if (box_id < 0 || static_cast<size_t>(box_id) >= box_configs_.size()) return;
  for (auto& u : units_) {
    if (u.assigned_box_id == box_id && u.state == PSUState::ACTIVE) {
      if (u.active_since > 0.0 && timestamp > u.active_since) {
        u.total_usage_time_s += (timestamp - u.active_since);
      }
      u.state = PSUState::READY;
      u.assigned_car_id = -1;
      u.active_since = 0.0;
    }
  }
}

//! @brief Checks if all PSUs for a box are ready for service.
//! @param box_id The pit box ID.
//! @return true if all assigned units are in READY state.
bool PitServiceUnitManager::is_service_ready(int box_id) const {
  if (box_id < 0 || static_cast<size_t>(box_id) >= box_configs_.size()) return false;
  const BoxPSUConfig& cfg = box_configs_[box_id];
  if (cfg.psu_ids.empty()) return false;
  for (int uid : cfg.psu_ids) {
    const auto* u = find_unit(uid);
    if (!u || u->state != PSUState::READY) return false;
  }
  return true;
}

//! @brief Estimates the total service time based on requested services.
//! @param box_id The pit box ID.
//! @param refuel Whether refueling is requested.
//! @param tires Whether tire change is requested.
//! @param repair Whether repair is requested.
//! @return Estimated service time in seconds.
double PitServiceUnitManager::estimated_service_time(int box_id, bool refuel, bool tires, bool repair) const {
  if (box_id < 0 || static_cast<size_t>(box_id) >= box_configs_.size()) return 0.0;
  const BoxPSUConfig& cfg = box_configs_[box_id];
  double max_time = 0.0;
  if (refuel && cfg.has_fuel_dispenser) max_time = std::max(max_time, 8.0);
  if (tires && cfg.has_tire_changer) max_time = std::max(max_time, 3.0);
  if (repair && cfg.has_compressor) max_time = std::max(max_time, 5.0);
  if (cfg.has_jack) max_time += 1.5;
  return max_time;
}

//! @brief Returns all PSU units assigned to a specific pit box.
//! @param box_id The pit box ID.
//! @return Vector of PitServiceUnit structs.
std::vector<PitServiceUnit> PitServiceUnitManager::units_for_box(int box_id) const {
  std::vector<PitServiceUnit> result;
  if (box_id < 0 || static_cast<size_t>(box_id) >= box_configs_.size()) return result;
  const BoxPSUConfig& cfg = box_configs_[box_id];
  for (int uid : cfg.psu_ids) {
    const auto* u = find_unit(uid);
    if (u) result.push_back(*u);
  }
  return result;
}

//! @brief Finds a PSU unit by its unique ID.
//! @param unit_id The unit ID to search for.
//! @return Pointer to the unit, or nullptr if not found.
const PitServiceUnit* PitServiceUnitManager::find_unit(int unit_id) const {
  for (const auto& u : units_) {
    if (u.unit_id == unit_id) return &u;
  }
  return nullptr;
}

//! @brief Updates fault detection for all active PSUs.
//!        Units with fault_count > 0 are transitioned to FAULT state.
//! @param timestamp Current race time in seconds.
void PitServiceUnitManager::update_faults(double timestamp) {
  for (auto& u : units_) {
    if (u.state == PSUState::ACTIVE && u.fault_count > 0 && u.state != PSUState::FAULT) {
      u.state = PSUState::FAULT;
      u.fault_message = "Overload detected";
    }
  }
}

//! @brief Sets or clears maintenance mode for a specific PSU.
//! @param unit_id The unit ID.
//! @param in_maintenance true to put in maintenance, false to restore to IDLE.
void PitServiceUnitManager::set_maintenance(int unit_id, bool in_maintenance) {
  auto* u = const_cast<PitServiceUnit*>(find_unit(unit_id));
  if (!u) return;
  if (in_maintenance) {
    u->state = PSUState::MAINTENANCE;
  } else if (u->state == PSUState::MAINTENANCE) {
    u->state = PSUState::IDLE;
    u->fault_message.clear();
  }
}

//! @brief Reinitializes all PSU units from the pit lane definition.
void PitServiceUnitManager::initialize_units() {
  create_units_for_boxes();
}

}
