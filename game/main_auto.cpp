#include "game/gameplay.h"
#include "engine/input/platform/auto_input.h"
#include <fstream>
#include <iostream>

// Project 0 — automated drive entry point
// Runs a scripted input sequence and logs telemetry to a text file.
// Useful for regression testing and offline analysis without user input.
int main() {
  std::ofstream log("data/telemetry/auto_drive_log.txt");
  std::cout.rdbuf(log.rdbuf());

  std::cout << "Project 0 - Auto Drive\n";

  p0::track::Track track;
  p0::simulation::Simulation sim;
  p0::telemetry::Telemetry tel;

  sim.set_track(track);

  p0::vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 0.0;

  sim.reset(initial);
  tel.clear();

  p0::gameplay::Gameplay gameplay(sim, tel, std::make_unique<p0::input::AutoInputManager>());

  // Auto-drive starts directly from the countdown to avoid waiting 16s
  // in the menu for the scripted sequence to reach the reset key.
  gameplay.set_phase(p0::gameplay::GameState::COUNTDOWN);
  gameplay.countdown().start_time =
      std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  gameplay.countdown().last_number = -1;
  gameplay.countdown().finished = false;

  gameplay.run();

  const auto& final_state = sim.state();
  std::cout << "Final speed: " << final_state.speed << " m/s\n";
  std::cout << "Final position: (" << final_state.position.x() << ", " << final_state.position.y() << ")\n";
  std::cout << "Final heading: " << final_state.heading << " rad\n";
  std::cout << "Distance along track: " << final_state.distance_along_track << " m\n";

  return 0;
}
