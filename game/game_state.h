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

// Built-in menu choices exposed as static data so the menu renderer can
// iterate them without owning copies. These are constant across sessions.
struct MenuChoices {
  std::vector<MenuOption> track_options;
  std::vector<MenuOption> lap_options;
  static const MenuChoices& defaults();
};

// Runtime state for a single-player session. Owned by Gameplay so multiple
// instances can coexist (each with their own menu / countdown / results).
struct MenuState {
  int current_menu_index = 0;
  int selected_track = 0;
  int selected_laps = 3;
};

struct CountdownState {
  double start_time = 0.0;
  double duration = 3.0;
  bool finished = false;
  int last_number = -1;
};

}