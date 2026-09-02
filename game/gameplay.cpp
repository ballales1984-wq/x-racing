// Project 0 — gameplay loop implementation (console/headless mode)
// Handles input polling, simulation stepping, lap timing, game flow, and console HUD.
#include "game/gameplay.h"
#include "game/game_state.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <windows.h>

namespace p0::gameplay {

const MenuChoices& MenuChoices::defaults() {
  static const MenuChoices kDefaults = [] {
    MenuChoices c;
    c.track_options = {
      {"Default Circuit", 0},
      {"Pit Circuit", 1}
    };
    c.lap_options = {
      {"1 Lap", 1},
      {"3 Laps", 3},
      {"5 Laps", 5},
      {"10 Laps", 10}
    };
    return c;
  }();
  return kDefaults;
}

static void enable_ansi_console() {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE) return;
  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode)) return;
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);
}

static std::string format_time(double time_sec) {
  if (time_sec <= 0.0) return "--:--.---";
  int minutes = static_cast<int>(time_sec / 60.0);
  double seconds = time_sec - minutes * 60.0;
  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::fixed << std::setprecision(3) << std::setw(8) << seconds;
  return oss.str();
}

static std::string save_path() {
  return "data/best_times.json";
}

Gameplay::Gameplay(simulation::Simulation& sim, telemetry::Telemetry& tel, std::unique_ptr<input::InputManager> input_manager)
    : sim_(sim), tel_(tel), input_manager_(std::move(input_manager)) {}

input::InputState Gameplay::poll_input() {
  return input_manager_->poll();
}

void Gameplay::reset_race() {
  state_.running = true;
  state_.current_lap = 0;
  state_.best_lap_time = 0.0;
  state_.current_lap_time = 0.0;
  state_.lap_times.clear();
  state_.off_track_warning = false;
  state_.off_track_frames = 0;
  last_lap_distance_ = 0.0;
  last_sim_time_ = 0.0;

  vehicle::VehicleState initial;
  initial.position = sim_.track().get_start_position();
  initial.heading = sim_.track().get_start_heading();
  initial.speed = 0.0;
  sim_.reset(initial);
  // Reset also clears telemetry so the new lap recording is clean.
  tel_.clear();
  // Drop any in-progress best times from the previous race; we'll persist
  // the new ones at RESULTS time.
  results_ = RaceResults{};
}

void Gameplay::handle_menu_input(const input::InputState& input) {
  const auto& choices = this->choices();
  const bool throttle_pressed = input.throttle > 0.5f && prev_input_.throttle <= 0.5f;
  const bool brake_pressed = input.brake > 0.5f && prev_input_.brake <= 0.5f;
  const bool upshift_pressed = input.upshift && !prev_input_.upshift;
  const bool box_pressed = input.enter_exit_box && !prev_input_.enter_exit_box;

  if (throttle_pressed) {
    if (menu_.current_menu_index == 0) {
      menu_.selected_track = (menu_.selected_track + 1) %
                              static_cast<int>(choices.track_options.size());
    } else {
      int opt_idx = -1;
      for (int i = 0; i < static_cast<int>(choices.lap_options.size()); ++i) {
        if (choices.lap_options[i].value == menu_.selected_laps) {
          opt_idx = i;
          break;
        }
      }
      if (opt_idx >= 0) {
        opt_idx = (opt_idx + 1) % static_cast<int>(choices.lap_options.size());
        menu_.selected_laps = choices.lap_options[opt_idx].value;
      }
    }
  }

  if (brake_pressed) {
    if (menu_.current_menu_index == 0) {
      menu_.selected_track =
          (menu_.selected_track + static_cast<int>(choices.track_options.size()) - 1) %
          static_cast<int>(choices.track_options.size());
    } else {
      int opt_idx = -1;
      for (int i = 0; i < static_cast<int>(choices.lap_options.size()); ++i) {
        if (choices.lap_options[i].value == menu_.selected_laps) {
          opt_idx = i;
          break;
        }
      }
      if (opt_idx >= 0) {
        opt_idx = (opt_idx + static_cast<int>(choices.lap_options.size()) - 1) %
                  static_cast<int>(choices.lap_options.size());
        menu_.selected_laps = choices.lap_options[opt_idx].value;
      }
    }
  }

  if (upshift_pressed) {
    menu_.current_menu_index = (menu_.current_menu_index + 1) % 2;
  }

  if (box_pressed) {
    menu_.current_menu_index = 1 - menu_.current_menu_index;
  }

  // ENTER starts the race (also accept R as a legacy shortcut).
  if (input_manager_->is_key_down(VK_RETURN) || (input.reset && !prev_input_.reset)) {
    countdown_.start_time = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    countdown_.last_number = -1;
    countdown_.finished = false;
    phase_ = GameState::COUNTDOWN;
  }
}

void Gameplay::handle_countdown() {
  auto now = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(now.time_since_epoch()).count()
                   - countdown_.start_time;
  int number = static_cast<int>(std::ceil(countdown_.duration - elapsed));
  if (number > 3) number = 3;
  if (number <= 0 && !countdown_.finished) {
    countdown_.finished = true;
    reset_race();
    phase_ = GameState::RACING;
  }
  countdown_.last_number = number;
}

void Gameplay::handle_racing(const simulation::SimulationResult& result) {
  if (result.off_track && !state_.off_track_warning) {
    state_.off_track_warning = true;
  }
  state_.off_track_frames = result.off_track ? state_.off_track_frames + 1 : 0;

  if (result.collision) {
    sim_.respawn();
    state_.off_track_frames = 0;
  }

  update_lap_timing(result);

  if (state_.current_lap >= menu_.selected_laps && state_.current_lap > 0) {
    phase_ = GameState::RESULTS;
    results_.completed = true;
    results_.completed_laps = static_cast<int>(state_.lap_times.size());
    results_.total_time = 0.0;
    results_.lap_times.clear();
    results_.lap_valid.clear();
    for (const auto& lt : state_.lap_times) {
      results_.lap_times.push_back(lt.lap_time);
      results_.lap_valid.push_back(lt.valid);
      if (lt.valid) {
        results_.total_time += lt.lap_time;
        if (results_.best_lap_time == 0.0 || lt.lap_time < results_.best_lap_time) {
          results_.best_lap_time = lt.lap_time;
        }
      }
    }
    if (state_.best_lap_time > 0.0 &&
        (results_.best_lap_time == 0.0 || state_.best_lap_time < results_.best_lap_time)) {
      results_.best_lap_time = state_.best_lap_time;
    }
    save_best_times();
  }
}

void Gameplay::save_best_times() {
  std::ofstream ofs(save_path());
  if (!ofs) return;
  const auto& choices = this->choices();
  ofs << "{\n";
  ofs << "  \"best_lap_time\": " << (results_.best_lap_time > 0.0 ? std::to_string(results_.best_lap_time) : "null") << ",\n";
  ofs << "  \"total_time\": " << std::to_string(results_.total_time) << ",\n";
  ofs << "  \"completed_laps\": " << results_.completed_laps << ",\n";
  ofs << "  \"track\": \"" << choices.track_options[menu_.selected_track].label << "\",\n";
  ofs << "  \"lap_count\": " << menu_.selected_laps << ",\n";
  ofs << "  \"lap_times\": [";
  for (size_t i = 0; i < results_.lap_times.size(); ++i) {
    if (i > 0) ofs << ", ";
    ofs << std::to_string(results_.lap_times[i]);
  }
  ofs << "],\n";
  ofs << "  \"lap_valid\": [";
  for (size_t i = 0; i < results_.lap_valid.size(); ++i) {
    if (i > 0) ofs << ", ";
    ofs << (results_.lap_valid[i] ? "true" : "false");
  }
  ofs << "]\n";
  ofs << "}\n";
}

void Gameplay::load_best_times() {
  std::ifstream ifs(save_path());
  if (!ifs) return;

  std::string content((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());

  auto find_double = [&](const std::string& key) -> double {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0.0;
    pos = content.find(':', pos);
    if (pos == std::string::npos) return 0.0;
    ++pos;
    while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) ++pos;
    try {
      return std::stod(content.substr(pos));
    } catch (const std::exception&) {
      return 0.0;
    }
  };

  auto find_int = [&](const std::string& key) -> int {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0;
    pos = content.find(':', pos);
    if (pos == std::string::npos) return 0;
    ++pos;
    while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) ++pos;
    try {
      return std::stoi(content.substr(pos));
    } catch (const std::exception&) {
      return 0;
    }
  };

  results_.best_lap_time = find_double("best_lap_time");
  results_.total_time = find_double("total_time");
  results_.completed_laps = find_int("completed_laps");
  results_.completed = results_.completed_laps > 0;

  // Parse lap_times array
  size_t lt_pos = content.find("\"lap_times\"");
  if (lt_pos != std::string::npos) {
    size_t bracket = content.find('[', lt_pos);
    if (bracket != std::string::npos) {
      size_t end = content.find(']', bracket);
      std::string arr = content.substr(bracket + 1, end - bracket - 1);
      std::istringstream iss(arr);
      std::string token;
      while (std::getline(iss, token, ',')) {
        try {
          results_.lap_times.push_back(std::stod(token));
        } catch (const std::exception&) {}
      }
    }
  }

  // Parse lap_valid array
  size_t lv_pos = content.find("\"lap_valid\"");
  if (lv_pos != std::string::npos) {
    size_t bracket = content.find('[', lv_pos);
    if (bracket != std::string::npos) {
      size_t end = content.find(']', bracket);
      std::string arr = content.substr(bracket + 1, end - bracket - 1);
      std::istringstream iss(arr);
      std::string token;
      while (std::getline(iss, token, ',')) {
        results_.lap_valid.push_back(token.find("true") != std::string::npos);
      }
    }
  }
}

void Gameplay::show_results() {
  enable_ansi_console();
  const auto& choices = this->choices();
  std::cout << "\033[2J\033[H";
  std::cout << "=== PROJECT 0 - Race Results ===\n\n";

  std::cout << "Track: " << choices.track_options[menu_.selected_track].label << "\n";
  std::cout << "Laps: " << menu_.selected_laps << "\n\n";

  std::cout << "--- Lap Times ---\n";
  std::cout << std::left << std::setw(6) << "Lap" << std::setw(12) << "Time" << "Status\n";
  std::cout << std::string(40, '-') << "\n";
  for (size_t i = 0; i < results_.lap_times.size(); ++i) {
    std::cout << std::setw(6) << (i + 1) << std::setw(12) << format_time(results_.lap_times[i])
              << (results_.lap_valid[i] ? "VALID" : "INVALID") << "\n";
  }

  std::cout << "\n--- Session Summary ---\n";
  std::cout << "Best Lap:  " << format_time(results_.best_lap_time) << "\n";
  std::cout << "Total Time: " << format_time(results_.total_time) << "\n";
  std::cout << "Valid Laps: " << std::count(results_.lap_valid.begin(), results_.lap_valid.end(), true)
            << " / " << results_.lap_times.size() << "\n";

  std::cout << "\nControls: R = Race Again | M = Main Menu | ESC = Quit\n";

  input::InputState input;
  input::InputState prev_result_input;
  bool waiting = true;
  while (waiting && state_.running) {
    input = poll_input();
    prev_result_input = input;

    if (input_manager_->is_key_down(VK_ESCAPE)) {
      state_.running = false;
      waiting = false;
    }
    if (input.reset && !prev_result_input.reset) {
      reset_race();
      countdown_.start_time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
      countdown_.last_number = -1;
      countdown_.finished = false;
      phase_ = GameState::COUNTDOWN;
      waiting = false;
    }
    if (input.throttle > 0.5 && prev_result_input.throttle <= 0.5) {
      phase_ = GameState::MENU;
      waiting = false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  prev_input_ = input;
}

void Gameplay::render_menu() {
  enable_ansi_console();
  const auto& choices = this->choices();
  std::cout << "\033[2J\033[H";
  std::cout << "╔══════════════════════════════════════╗\n";
  std::cout << "║        X-RACING - PROJECT 0          ║\n";
  std::cout << "║        Racing Simulator MVP          ║\n";
  std::cout << "╚══════════════════════════════════════╝\n\n";

  std::cout << "--- Track ---\n";
  for (size_t i = 0; i < choices.track_options.size(); ++i) {
    std::string marker = (static_cast<int>(i) == menu_.selected_track) ? "> " : "  ";
    std::cout << marker << choices.track_options[i].label << "\n";
  }

  std::cout << "\n--- Laps ---\n";
  for (size_t i = 0; i < choices.lap_options.size(); ++i) {
    std::string marker = (choices.lap_options[i].value == menu_.selected_laps) ? "> " : "  ";
    std::cout << marker << choices.lap_options[i].label << "\n";
  }

  std::cout << "\nControls: W/S = Cycle | B = Switch Group | ENTER / R = Start | ESC = Quit\n";
}

void Gameplay::render_countdown() {
  enable_ansi_console();
  const auto& choices = this->choices();
  auto now = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(now.time_since_epoch()).count()
                   - countdown_.start_time;
  int number = static_cast<int>(std::ceil(countdown_.duration - elapsed));
  if (number > 3) number = 3;
  if (number < 0) number = 0;
  countdown_.last_number = number;

  std::cout << "\033[2J\033[H";
  std::cout << "╔══════════════════════════════════════╗\n";
  std::cout << "║          X-RACING - GET READY!       ║\n";
  std::cout << "╚══════════════════════════════════════╝\n\n";

  std::string display;
  if (number > 0) {
    display = std::to_string(number);
  } else {
    display = "GO!";
  }

  std::cout << "         ╔═══════════╗\n";
  std::cout << "         ║";
  int padding = (11 - static_cast<int>(display.length())) / 2;
  for (int i = 0; i < padding; ++i) std::cout << " ";
  std::cout << display;
  for (int i = 0; i < 11 - padding - static_cast<int>(display.length()); ++i) std::cout << " ";
  std::cout << "║\n";
  std::cout << "         ╚═══════════╝\n\n";

  std::cout << "Track: " << choices.track_options[menu_.selected_track].label << "\n";
  std::cout << "Laps: " << menu_.selected_laps << "\n";
  std::cout << "ESC = Back to Menu\n";
}

void Gameplay::update_lap_timing(const simulation::SimulationResult& result) {
  const double track_len = sim_.track().length();
  const double dt = (std::max)(0.0, result.time - last_sim_time_);
  last_sim_time_ = result.time;

  if (result.state.lap > state_.current_lap) {
    if (state_.current_lap >= 0) {
      LapTime lt;
      lt.lap_time = state_.current_lap_time;
      lt.valid = !state_.off_track_warning;
      state_.lap_times.push_back(lt);

      if (lt.valid && (state_.best_lap_time == 0.0 || lt.lap_time < state_.best_lap_time)) {
        state_.best_lap_time = lt.lap_time;
      }
    }
    state_.current_lap = result.state.lap;
    state_.current_lap_time = 0.0;
    state_.off_track_warning = false;
    tel_.mark_lap(result.state.lap);
  }

  state_.current_lap_time += dt;
  last_lap_distance_ = result.state.distance_along_track;
}

void Gameplay::render_console(const simulation::SimulationResult& result) {
  const auto& s = result.state;
  const double speed_kmh = s.speed * 3.6;
  const Vec2 lateral_axis(-std::sin(s.heading), std::cos(s.heading));
  const double lateral_g = s.acceleration.dot(lateral_axis) / kGravity;

  std::cout << "\033[2J\033[H";
  std::cout << "=== X-RACING - PROJECT 0 ===\n\n";

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "Speed:      " << std::setw(6) << speed_kmh << " km/h\n";
  std::cout << "RPM:        " << std::setw(8) << s.rpm << "\n";
  std::cout << "Gear:       " << s.gear << "\n";
  std::cout << "Throttle:   " << std::setw(5) << s.throttle * 100.0 << "%\n";
  std::cout << "Brake:      " << std::setw(5) << s.brake * 100.0 << "%\n";
  std::cout << "Steer:      " << std::setw(6) << s.steer_angle * kRadToDeg << " deg\n";

  std::cout << "\n--- Telemetry ---\n";
  std::cout << "Lap:        " << s.lap << " / " << menu_.selected_laps << "\n";
  std::cout << "Lap Time:   " << std::setw(6) << state_.current_lap_time << " s\n";
  if (state_.best_lap_time > 0.0) {
    std::cout << "Best Lap:   " << std::setw(6) << state_.best_lap_time << " s\n";
  }
  std::cout << "Distance:   " << std::setw(8) << s.distance_along_track << " m / " << sim_.track().length() << " m\n";

  std::cout << "\n--- Tire Status ---\n";
  std::cout << "Front Temp: " << std::setw(6) << s.front_tire_temp << " K\n";
  std::cout << "Rear Temp:  " << std::setw(6) << s.rear_tire_temp << " K\n";
  std::cout << "Front Wear: " << std::setw(5) << (1.0 - s.front_tire_wear) * 100.0 << "%\n";
  std::cout << "Rear Wear:  " << std::setw(5) << (1.0 - s.rear_tire_wear) * 100.0 << "%\n";

  std::cout << "\n--- Aero ---\n";
  std::cout << "Drag:       " << std::setw(8) << s.aero_drag << " N\n";
  std::cout << "Downforce:  " << std::setw(8) << s.aero_downforce << " N\n";

  std::cout << "\n--- Slip Angles ---\n";
  std::cout << "Front:      " << std::setw(6) << s.front_slip_angle * kRadToDeg << " deg\n";
  std::cout << "Rear:       " << std::setw(6) << s.rear_slip_angle * kRadToDeg << " deg\n";

  if (result.off_track) {
    const double secs_off = state_.off_track_frames / 60.0;
    std::cout << "\n*** OFF TRACK (" << std::fixed << std::setprecision(1) << secs_off << " s) ***\n";
  }
  if (result.collision) {
    std::cout << "*** COLLISION - auto respawn... ***\n";
  }
  if (state_.off_track_warning) {
    std::cout << "*** LAP INVALID ***\n";
  }

  std::cout << "\nControls: WASD/Arrows = Drive | Shift = Upshift | Ctrl = Downshift | R = Reset | ESC = Quit\n";
}

void Gameplay::run() {
  enable_ansi_console();
  state_ = GameplayState{};
  load_best_times();

  const double target_dt = 1.0 / 60.0;
  auto last_time = std::chrono::high_resolution_clock::now();

  while (state_.running) {
    auto current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = current_time - last_time;
    last_time = current_time;

    input::InputState input = poll_input();

    if (input_manager_->is_key_down(VK_ESCAPE) && phase_ != GameState::MENU) {
      phase_ = GameState::MENU;
      continue;
    }
    if (input_manager_->is_key_down(VK_ESCAPE) && phase_ == GameState::MENU) {
      state_.running = false;
      break;
    }

    switch (phase_) {
      case GameState::MENU:
        handle_menu_input(input);
        render_menu();
        break;

      case GameState::COUNTDOWN:
        handle_countdown();
        render_countdown();
        break;

      case GameState::RACING: {
      if (input.reset && !prev_input_.reset) {
        sim_.respawn();
        state_.off_track_warning = true;
        state_.off_track_frames = 0;
        continue;
      }

        simulation::SimulationResult result = sim_.step(input);
        handle_racing(result);
        render_console(result);
        tel_.record(result.state, target_dt);
        break;
      }

      case GameState::RESULTS:
        show_results();
        break;
    }

    std::this_thread::sleep_for(std::chrono::duration<double>(target_dt));
    prev_input_ = input;
  }

  std::cout << "\nSession ended.\n";
  std::cout << "Best lap: " << state_.best_lap_time << " s\n";

  for (size_t i = 0; i < state_.lap_times.size(); ++i) {
    std::cout << "Lap " << (i + 1) << ": " << state_.lap_times[i].lap_time
              << " s (" << (state_.lap_times[i].valid ? "valid" : "invalid") << ")\n";
  }

  tel_.save_csv("data/telemetry/unity_state.csv");
  std::cout << "Telemetry saved to data/telemetry/unity_state.csv\n";
}

}