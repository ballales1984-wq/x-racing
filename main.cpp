// Project 0 — DX11 renderer entry point
// Builds a default track, simulation and vehicle state, then launches
// the DirectX 11 renderer which handles input, physics stepping
// and drawing with skinned mesh animation.
#include "renderer/dx11_renderer.h"
#include "simulation/simulation.h"
#include "track/track.h"
#include "vehicle/vehicle.h"

using namespace p0;

int main() {
  track::Track track;
  simulation::SimulationParams params;
  simulation::Simulation sim(params);
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 0.0;
  sim.reset(initial);

  renderer::DX11Config cfg;
  cfg.width = 1280;
  cfg.height = 720;
  renderer::DX11Renderer renderer(sim, cfg);
  if (!renderer.initialize()) {
    return 1;
  }
  renderer.run();

  return 0;
}
