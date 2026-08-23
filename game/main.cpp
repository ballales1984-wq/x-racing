#include "game/gameplay.h"
#include "engine/input/platform/windows_input.h"
#include <iostream>

int main() {
  std::cout << "Project 0 - Gameplay\n";
  std::cout << "Initializing simulation...\n";

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

  p0::input::WindowsInputManager windows_input;
  p0::gameplay::Gameplay gameplay(sim, tel, std::make_unique<p0::input::WindowsInputManager>(std::move(windows_input)));
  gameplay.run();

  std::cout << "Exiting.\n";
  return 0;
}
