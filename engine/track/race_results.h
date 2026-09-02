// Project 0 — Race results database (leaderboard, personal bests, track records)
// Namespace: p0::track
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>

namespace p0::track {

// Single lap time entry recorded during a race session.
struct LapTimeEntry {
  double lap_time = 0.0;  // s
  bool valid = true;
  int lap_number = 0;
  double timestamp = 0.0;  // s, session time when lap was completed
};

// Full race result for a single car in a single session.
struct RaceResult {
  std::string race_id;
  std::string track_id;
  std::string track_name;
  int car_id = 0;
  std::string driver_name;
  int completed_laps = 0;
  double total_time = 0.0;  // s
  double best_lap_time = 0.0;  // s
  std::vector<LapTimeEntry> lap_times;
  std::string session_date;
  bool finished = false;
  bool dnf = false;
  std::string dnf_reason;
};

// Aggregated leaderboard entry for a driver on a track.
struct LeaderboardEntry {
  std::string driver_name;
  int car_id = 0;
  double best_lap_time = 0.0;  // s
  double total_time = 0.0;  // s
  int races_completed = 0;
  int total_valid_laps = 0;
  double avg_lap_time = 0.0;  // s
  int position = 0;
};

// Persistent in-memory database of race results.
// Supports leaderboard queries, personal bests, and track records with caching.
class ResultsDatabase {
 public:
  // Add a new race result to the database.
  void add_result(const RaceResult& result);
  // Remove all stored results.
  void clear();

  const std::vector<RaceResult>& all_results() const { return results_; }
  // Filter results by track ID.
  std::vector<RaceResult> results_for_track(const std::string& track_id) const;
  // Filter results by driver name.
  std::vector<RaceResult> results_for_driver(const std::string& driver_name) const;

  // Leaderboard sorted by best lap time (optionally filtered by track).
  std::vector<LeaderboardEntry> leaderboard_by_best_lap(const std::string& track_id = "") const;
  std::vector<LeaderboardEntry> leaderboard_by_total_time(const std::string& track_id = "") const;
  std::vector<LeaderboardEntry> leaderboard_by_avg_lap(const std::string& track_id = "") const;

  // Best lap time for a specific driver (optionally on a specific track).
  std::optional<RaceResult> personal_best(const std::string& driver_name, const std::string& track_id = "") const;
  // Overall track record (fastest lap across all drivers).
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
