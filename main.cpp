// Project 0 — renderer entry point
// Builds a default track, simulation and vehicle state, then launches
// the real-time Win32 renderer which handles input, physics stepping
// and drawing.
#include "renderer/renderer.h"
#include "simulation/simulation.h"
#include "track/track.h"
#include "vehicle/vehicle.h"

using namespace p0;

int main() {
  // Build the default track and the simulation bound to it.
  track::Track track;
  simulation::SimulationParams params;
  simulation::Simulation sim(params);
  sim.set_track(track);

  // Place the car at the start line, stationary.
  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 0.0;
  sim.reset(initial);

  // Run the real-time renderer (handles input, stepping and drawing).
  renderer::Renderer renderer(sim);
  renderer.run();

  return 0;
}
