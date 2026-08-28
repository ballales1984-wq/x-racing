#pragma once

#include <string>
#include <vector>
#include <algorithm>

namespace p0::gameplay {

enum class GameState {
  MENU,
  COUNTDOWN,
  RACING,
  RESULTS
};

struct SessionConfig {
  int lap_count = 3;
  int track_type = 0;
  std::string track_name;
};

struct RaceResults {
  bool completed = false;
  double best_lap_time = 0.0;
  double total_time = 0.0;
  int completed_laps = 0;
  std::vector<double> lap_times;
  std::vector<bool> lap_valid;
};

struct MenuOption {
  std::string label;
  int value;
};

inline int current_menu_index = 0;
inline std::vector<MenuOption> track_options = {
  {"Default Circuit", 0},
  {"Pit Circuit", 1}
};

inline std::vector<MenuOption> lap_options = {
  {"1 Lap", 1},
  {"3 Laps", 3},
  {"5 Laps", 5},
  {"10 Laps", 10}
};

inline int selected_track = 0;
inline int selected_laps = 3;

inline GameState state = GameState::MENU;
inline double countdown_start_time = 0.0;
inline const double countdown_duration = 3.0;
inline bool countdown_finished = false;
inline int countdown_last_number = -1;

inline RaceResults results;

}
