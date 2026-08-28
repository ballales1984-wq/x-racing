#include "race_results.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace p0::track {

void ResultsDatabase::add_result(const RaceResult& result) {
  results_.push_back(result);
  invalidate_cache();
}

void ResultsDatabase::clear() {
  results_.clear();
  invalidate_cache();
}

void ResultsDatabase::invalidate_cache() const {
  cache_valid_ = false;
  leaderboard_cache_.clear();
}

std::vector<RaceResult> ResultsDatabase::results_for_track(const std::string& track_id) const {
  std::vector<RaceResult> filtered;
  for (const auto& r : results_) {
    if (r.track_id == track_id) {
      filtered.push_back(r);
    }
  }
  return filtered;
}

std::vector<RaceResult> ResultsDatabase::results_for_driver(const std::string& driver_name) const {
  std::vector<RaceResult> filtered;
  for (const auto& r : results_) {
    if (r.driver_name == driver_name) {
      filtered.push_back(r);
    }
  }
  return filtered;
}

std::vector<LeaderboardEntry> ResultsDatabase::compute_leaderboard(const std::string& track_id) const {
  std::unordered_map<std::string, LeaderboardEntry> entries;

  for (const auto& r : results_) {
    if (!track_id.empty() && r.track_id != track_id) continue;
    if (!r.finished || r.best_lap_time <= 0.0) continue;

    auto& entry = entries[r.driver_name];
    entry.driver_name = r.driver_name;
    entry.car_id = r.car_id;
    entry.races_completed += 1;
    entry.total_valid_laps += static_cast<int>(r.lap_times.size());

    if (entry.best_lap_time == 0.0 || r.best_lap_time < entry.best_lap_time) {
      entry.best_lap_time = r.best_lap_time;
    }
    if (r.total_time > 0.0) {
      entry.total_time = entry.total_time + r.total_time;
    }
  }

  for (auto& [name, entry] : entries) {
    if (entry.total_valid_laps > 0) {
      entry.avg_lap_time = entry.total_time / entry.total_valid_laps;
    }
  }

  std::vector<LeaderboardEntry> sorted;
  sorted.reserve(entries.size());
  for (auto& [name, entry] : entries) {
    sorted.push_back(entry);
  }
  return sorted;
}

std::vector<LeaderboardEntry> ResultsDatabase::leaderboard_by_best_lap(const std::string& track_id) const {
  return sort_by_best_lap(compute_leaderboard(track_id));
}

std::vector<LeaderboardEntry> ResultsDatabase::leaderboard_by_total_time(const std::string& track_id) const {
  return sort_by_total_time(compute_leaderboard(track_id));
}

std::vector<LeaderboardEntry> ResultsDatabase::leaderboard_by_avg_lap(const std::string& track_id) const {
  return sort_by_avg_lap(compute_leaderboard(track_id));
}

std::vector<LeaderboardEntry> ResultsDatabase::sort_by_best_lap(const std::vector<LeaderboardEntry>& entries) const {
  std::vector<LeaderboardEntry> sorted = entries;
  std::sort(sorted.begin(), sorted.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
    if (a.best_lap_time <= 0.0) return false;
    if (b.best_lap_time <= 0.0) return true;
    return a.best_lap_time < b.best_lap_time;
  });
  for (size_t i = 0; i < sorted.size(); ++i) {
    sorted[i].position = static_cast<int>(i + 1);
  }
  return sorted;
}

std::vector<LeaderboardEntry> ResultsDatabase::sort_by_total_time(const std::vector<LeaderboardEntry>& entries) const {
  std::vector<LeaderboardEntry> sorted = entries;
  std::sort(sorted.begin(), sorted.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
    if (a.total_time <= 0.0) return false;
    if (b.total_time <= 0.0) return true;
    return a.total_time < b.total_time;
  });
  for (size_t i = 0; i < sorted.size(); ++i) {
    sorted[i].position = static_cast<int>(i + 1);
  }
  return sorted;
}

std::vector<LeaderboardEntry> ResultsDatabase::sort_by_avg_lap(const std::vector<LeaderboardEntry>& entries) const {
  std::vector<LeaderboardEntry> sorted = entries;
  std::sort(sorted.begin(), sorted.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
    if (a.avg_lap_time <= 0.0) return false;
    if (b.avg_lap_time <= 0.0) return true;
    return a.avg_lap_time < b.avg_lap_time;
  });
  for (size_t i = 0; i < sorted.size(); ++i) {
    sorted[i].position = static_cast<int>(i + 1);
  }
  return sorted;
}

std::optional<RaceResult> ResultsDatabase::personal_best(const std::string& driver_name, const std::string& track_id) const {
  std::optional<RaceResult> best;
  for (const auto& r : results_) {
    if (r.driver_name != driver_name) continue;
    if (!track_id.empty() && r.track_id != track_id) continue;
    if (!r.finished || r.best_lap_time <= 0.0) continue;
    if (!best || r.best_lap_time < best->best_lap_time) {
      best = r;
    }
  }
  return best;
}

std::optional<RaceResult> ResultsDatabase::track_record(const std::string& track_id) const {
  std::optional<RaceResult> record;
  for (const auto& r : results_) {
    if (!track_id.empty() && r.track_id != track_id) continue;
    if (!r.finished || r.best_lap_time <= 0.0) continue;
    if (!record || r.best_lap_time < record->best_lap_time) {
      record = r;
    }
  }
  return record;
}

}
