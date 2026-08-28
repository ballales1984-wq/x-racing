#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>

namespace p0::track {

struct LapTimeEntry {
  double lap_time = 0.0;
  bool valid = true;
  int lap_number = 0;
  double timestamp = 0.0;
};

struct RaceResult {
  std::string race_id;
  std::string track_id;
  std::string track_name;
  int car_id = 0;
  std::string driver_name;
  int completed_laps = 0;
  double total_time = 0.0;
  double best_lap_time = 0.0;
  std::vector<LapTimeEntry> lap_times;
  std::string session_date;
  bool finished = false;
  bool dnf = false;
  std::string dnf_reason;
};

struct LeaderboardEntry {
  std::string driver_name;
  int car_id = 0;
  double best_lap_time = 0.0;
  double total_time = 0.0;
  int races_completed = 0;
  int total_valid_laps = 0;
  double avg_lap_time = 0.0;
  int position = 0;
};

class ResultsDatabase {
 public:
  void add_result(const RaceResult& result);
  void clear();

  const std::vector<RaceResult>& all_results() const { return results_; }
  std::vector<RaceResult> results_for_track(const std::string& track_id) const;
  std::vector<RaceResult> results_for_driver(const std::string& driver_name) const;

  std::vector<LeaderboardEntry> leaderboard_by_best_lap(const std::string& track_id = "") const;
  std::vector<LeaderboardEntry> leaderboard_by_total_time(const std::string& track_id = "") const;
  std::vector<LeaderboardEntry> leaderboard_by_avg_lap(const std::string& track_id = "") const;

  std::optional<RaceResult> personal_best(const std::string& driver_name, const std::string& track_id = "") const;
  std::optional<RaceResult> track_record(const std::string& track_id = "") const;

 private:
  std::vector<RaceResult> results_;
  mutable std::unordered_map<std::string, std::vector<LeaderboardEntry>> leaderboard_cache_;
  mutable bool cache_valid_ = false;

  void invalidate_cache() const;
  std::vector<LeaderboardEntry> compute_leaderboard(const std::string& track_id) const;
  std::vector<LeaderboardEntry> sort_by_best_lap(const std::vector<LeaderboardEntry>& entries) const;
  std::vector<LeaderboardEntry> sort_by_total_time(const std::vector<LeaderboardEntry>& entries) const;
  std::vector<LeaderboardEntry> sort_by_avg_lap(const std::vector<LeaderboardEntry>& entries) const;
};

}
