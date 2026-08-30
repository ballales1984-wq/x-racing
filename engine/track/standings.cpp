#include "standings.h"

namespace p0::track {

void StandingsTracker::reset() {
  car_data_.clear();
  leader_lap_count_ = 0;
  dirty_ = true;
}

void StandingsTracker::clear_dynamic_data() {
  for (auto& [id, data] : car_data_) {
    data.laps_completed = 0;
    data.total_time = 0.0;
    data.best_lap_time = 0.0;
    data.pit_stops = 0;
    data.finished = false;
    data.dnf = false;
    data.finish_time = 0.0;
    data.dnf_reason.clear();
    data.status = p0::race::RaceSessionState::PREGAME;
  }
  leader_lap_count_ = 0;
  dirty_ = true;
}

void StandingsTracker::set_grid(const std::vector<p0::race::CarAssignment>& assignments,
                                const std::vector<GridSlot>& grid_slots) {
  car_data_.clear();
  for (const auto& a : assignments) {
    CarData data;
    data.car_id = a.car_id;
    data.grid_position = a.grid_slot;
    car_data_[a.car_id] = data;
  }
  dirty_ = true;
}

void StandingsTracker::update(const std::unordered_map<int, double>& car_finished_times,
                              const std::unordered_map<int, int>& car_lap_counts) {
  for (auto& [id, data] : car_data_) {
    auto it = car_lap_counts.find(id);
    if (it != car_lap_counts.end()) {
      data.laps_completed = it->second;
    }
    auto ft = car_finished_times.find(id);
    if (ft != car_finished_times.end()) {
      data.finished = true;
      data.finish_time = ft->second;
    }
    if (data.laps_completed > leader_lap_count_) {
      leader_lap_count_ = data.laps_completed;
    }
  }
  dirty_ = true;
}

std::vector<CarStandingsEntry> StandingsTracker::current_standings() const {
  std::vector<CarStandingsEntry> result;
  result.reserve(car_data_.size());

  for (const auto& [id, data] : car_data_) {
    CarStandingsEntry entry;
    entry.car_id = data.car_id;
    entry.gap_to_leader_s = 0.0;
    entry.best_lap_time = data.best_lap_time;
    entry.laps_completed = data.laps_completed;
    entry.pit_stops = data.pit_stops;
    entry.finished = data.finished;
    entry.dnf = data.dnf;
    entry.dnf_reason = data.dnf_reason;
    entry.status = data.status;
    result.push_back(entry);
  }

  std::stable_sort(result.begin(), result.end(), [](const CarStandingsEntry& a, const CarStandingsEntry& b) {
    if (a.dnf != b.dnf) return !a.dnf;
    if (a.finished != b.finished) return a.finished;
    if (a.laps_completed != b.laps_completed) return a.laps_completed > b.laps_completed;
    return a.best_lap_time < b.best_lap_time && a.best_lap_time > 0.0;
  });

  if (!result.empty()) {
    int leader_lap = result[0].laps_completed;
    double leader_time = result[0].best_lap_time;
    for (size_t i = 0; i < result.size(); ++i) {
      result[i].position = static_cast<int>(i + 1);
      if (i == 0) {
        result[i].gap_to_leader_s = 0.0;
      } else {
        if (result[i].laps_completed < leader_lap) {
          result[i].gap_to_leader_s = static_cast<double>(leader_lap - result[i].laps_completed);
        } else {
          result[i].gap_to_leader_s = result[i].best_lap_time - leader_time;
          if (result[i].gap_to_leader_s < 0.0) result[i].gap_to_leader_s = 0.0;
        }
      }
    }
  }

  return result;
}

CarStandingsEntry StandingsTracker::get_car(int car_id) const {
  auto it = car_data_.find(car_id);
  if (it == car_data_.end()) return CarStandingsEntry{};
  auto standings = current_standings();
  for (const auto& entry : standings) {
    if (entry.car_id == car_id) return entry;
  }
  return CarStandingsEntry{};
}

void StandingsTracker::record_lap(int car_id, double lap_time, bool valid) {
  auto it = car_data_.find(car_id);
  if (it == car_data_.end()) return;
  if (valid && lap_time > 0.0) {
    if (it->second.best_lap_time <= 0.0 || lap_time < it->second.best_lap_time) {
      it->second.best_lap_time = lap_time;
    }
    it->second.total_time += lap_time;
  }
  dirty_ = true;
}

void StandingsTracker::mark_finished(int car_id, double finish_time) {
  auto it = car_data_.find(car_id);
  if (it == car_data_.end()) return;
  it->second.finished = true;
  it->second.finish_time = finish_time;
  dirty_ = true;
}

void StandingsTracker::mark_dnf(int car_id, const std::string& reason) {
  auto it = car_data_.find(car_id);
  if (it == car_data_.end()) return;
  it->second.dnf = true;
  it->second.dnf_reason = reason;
  dirty_ = true;
}

void StandingsTracker::increment_pit_stops(int car_id) {
  auto it = car_data_.find(car_id);
  if (it == car_data_.end()) return;
  it->second.pit_stops++;
  dirty_ = true;
}

void StandingsTracker::set_car_status(int car_id, p0::race::RaceSessionState status) {
  auto it = car_data_.find(car_id);
  if (it == car_data_.end()) return;
  it->second.status = status;
  dirty_ = true;
}

void StandingsTracker::recompute_positions() {
  dirty_ = false;
}

}
