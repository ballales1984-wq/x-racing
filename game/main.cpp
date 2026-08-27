#include "game/gameplay.h"
#include "engine/input/platform/windows_input.h"
#include <iostream>

// Project 0 — interactive gameplay entry point
// Builds a default track, simulation and telemetry recorder, then runs
// the real-time gameplay loop with Windows keyboard input.
int main() {
  std::cout << "Project 0 - Gameplay\n";
  std::cout << "Initializing simulation...\n";

  // Create the default track, simulation and telemetry recorder.
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

  p0::gameplay::Gameplay gameplay(sim, tel, std::make_unique<p0::input::WindowsInputManager>());
  gameplay.run();

  std::cout << "Exiting.\n";
  return 0;
}
