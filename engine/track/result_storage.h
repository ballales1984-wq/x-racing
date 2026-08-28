#pragma once

#include <string>
#include <vector>
#include "race_results.h"

namespace p0::track {

class ResultStorage {
 public:
  static bool save_results(const std::string& filepath, const ResultsDatabase& db);
  static bool load_results(const std::string& filepath, ResultsDatabase& db);

  static bool save_race_result(const std::string& filepath, const RaceResult& result);
  static std::optional<RaceResult> load_race_result(const std::string& filepath);

  static std::string default_results_path();
  static std::string generate_race_id(const std::string& track_id, int car_id);

  static std::string current_timestamp();
  static std::string format_time(double time_sec);
};

}
