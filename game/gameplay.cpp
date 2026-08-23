#include "game/gameplay.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <thread>
#include <windows.h>

namespace p0::gameplay {

Gameplay::Gameplay(simulation::Simulation& sim, telemetry::Telemetry& tel, std::unique_ptr<input::InputManager> input_manager)
    : sim_(sim), tel_(tel), input_manager_(std::move(input_manager)) {}

input::InputState Gameplay::poll_input() {
  return input_manager_->poll();
}

void Gameplay::update_lap_timing(const simulation::SimulationResult& result) {
  const double track_len = sim_.track().length();

  if (result.state.lap > state_.current_lap) {
    if (state_.current_lap > 0) {
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
  }

  state_.current_lap_time += result.time;
  last_lap_distance_ = result.state.distance_along_track;
}

void Gameplay::render_console(const simulation::SimulationResult& result) {
  const auto& s = result.state;
  const double speed_kmh = s.speed * 3.6;
  const double lateral_g = s.lateral_velocity * s.yaw_rate / kGravity;

  std::cout << "\033[2J\033[H";
  std::cout << "=== PROJECT 0 - Gameplay ===\n\n";

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "Speed:      " << std::setw(6) << speed_kmh << " km/h\n";
  std::cout << "RPM:        " << std::setw(8) << s.rpm << "\n";
  std::cout << "Gear:       " << s.gear << "\n";
  std::cout << "Throttle:   " << std::setw(5) << s.throttle * 100.0 << "%\n";
  std::cout << "Brake:      " << std::setw(5) << s.brake * 100.0 << "%\n";
  std::cout << "Steer:      " << std::setw(6) << s.steer_angle * kRadToDeg << " deg\n";

  std::cout << "\n--- Telemetry ---\n";
  std::cout << "Lap:        " << s.lap << "\n";
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
    std::cout << "\n*** OFF TRACK ***\n";
  }
  if (state_.off_track_warning) {
    std::cout << "\n*** LAP INVALID ***\n";
  }

  std::cout << "\nControls: WASD/Arrows = Drive | Shift = Upshift | Ctrl = Downshift | R = Reset | ESC = Quit\n";
}

void Gameplay::run() {
  state_.running = true;
  state_.current_lap = 0;
  state_.best_lap_time = 0.0;
  state_.current_lap_time = 0.0;
  last_lap_distance_ = 0.0;

  vehicle::VehicleState initial;
  initial.position = sim_.track().get_start_position();
  initial.heading = sim_.track().get_start_heading();
  initial.speed = 0.0;
  sim_.reset(initial);
  tel_.clear();

  const double target_dt = 1.0 / 60.0;
  auto last_time = std::chrono::high_resolution_clock::now();

  while (state_.running) {
    auto current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = current_time - last_time;
    last_time = current_time;

    input::InputState input = poll_input();

    if (input.reset) {
      sim_.reset(initial);
      tel_.clear();
      state_.current_lap_time = 0.0;
      state_.off_track_warning = false;
      last_lap_distance_ = 0.0;
    }

    if (input_manager_->is_key_down(VK_ESCAPE)) {
      state_.running = false;
      break;
    }

    simulation::SimulationResult result = sim_.step(input);

    if (result.off_track && !state_.off_track_warning) {
      state_.off_track_warning = true;
    }

    update_lap_timing(result);
    render_console(result);

    tel_.record(result.state, target_dt);

    std::this_thread::sleep_for(std::chrono::duration<double>(target_dt));
  }

  std::cout << "\nSession ended.\n";
  std::cout << "Best lap: " << state_.best_lap_time << " s\n";

  for (size_t i = 0; i < state_.lap_times.size(); ++i) {
    std::cout << "Lap " << (i + 1) << ": " << state_.lap_times[i].lap_time
              << " s (" << (state_.lap_times[i].valid ? "valid" : "invalid") << ")\n";
  }
}

}
