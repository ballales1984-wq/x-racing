#pragma once

#include "race_config.h"
#include "track_data.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace p0::track {

struct CarStandingsEntry {
  int car_id = 0;
  int position = 0;
  double gap_to_leader_s = 0.0;
  double best_lap_time = 0.0;
  int laps_completed = 0;
  int pit_stops = 0;
  bool finished = false;
  bool dnf = false;
  std::string dnf_reason;
  p0::race::RaceSessionState status = p0::race::RaceSessionState::PREGAME;
};

struct StandingsSnapshot {
  double timestamp = 0.0;
  std::vector<CarStandingsEntry> entries;
};

class StandingsTracker {
 public:
  void reset();
  void clear_dynamic_data();
  void set_grid(const std::vector<p0::race::CarAssignment>& assignments,
                const std::vector<GridSlot>& grid_slots);
  void update(const std::unordered_map<int, double>& car_finished_times,
              const std::unordered_map<int, int>& car_lap_counts);
  std::vector<CarStandingsEntry> current_standings() const;
  CarStandingsEntry get_car(int car_id) const;
  void record_lap(int car_id, double lap_time, bool valid);
  void mark_finished(int car_id, double finish_time);
  void mark_dnf(int car_id, const std::string& reason);
  void increment_pit_stops(int car_id);
  void set_car_status(int car_id, p0::race::RaceSessionState status);
  int leader_lap() const { return leader_lap_count_; }

 private:
  void recompute_positions();

  struct CarData {
    int car_id = 0;
    int grid_position = 0;
    int laps_completed = 0;
    double total_time = 0.0;
    double best_lap_time = 0.0;
    int pit_stops = 0;
    bool finished = false;
    bool dnf = false;
    double finish_time = 0.0;
    std::string dnf_reason;
    p0::race::RaceSessionState status = p0::race::RaceSessionState::PREGAME;
  };

  std::unordered_map<int, CarData> car_data_;
  int leader_lap_count_ = 0;
  bool dirty_ = true;
};

}
