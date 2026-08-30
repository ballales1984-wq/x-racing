#pragma once

#include "common.h"
#include "track/race_config.h"
#include "track/standings.h"
#include <string>
#include <vector>

namespace p0::ui {

struct HudConfig {
  int screen_width = 1280;
  int screen_height = 720;
  bool show_standings = true;
  bool show_track_map = true;
  bool show_lap_times = true;
  bool show_speed = true;
  bool show_rpm = true;
};

struct SpeedoState {
  double speed_kmh = 0.0;
  double rpm = 0.0;
  int gear = 1;
  double max_rpm = 8000.0;
};

struct LapInfo {
  int current_lap = 0;
  int total_laps = 0;
  double current_lap_time = 0.0;
  double best_lap_time = 0.0;
  double last_lap_time = 0.0;
  std::vector<double> lap_times;
};

struct HudState {
  SpeedoState speedo;
  LapInfo lap;
  int position = 1;
  int total_cars = 1;
  double gap_to_leader = 0.0;
  double gap_to_ahead = 0.0;
  p0::race::FlagState flag = p0::race::FlagState::GREEN;
  std::vector<p0::track::CarStandingsEntry> standings;
};

class Hud {
 public:
  Hud();

  void set_config(const HudConfig& config);
  const HudConfig& config() const { return config_; }

  void update(const HudState& state);
  const HudState& state() const { return state_; }

  std::string format_time(double seconds) const;
  std::string format_gap(double seconds) const;

 private:
  HudConfig config_;
  HudState state_;
};

}
