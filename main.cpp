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

  auto grid = track.generate_grid(track::GridLayout::TWO_COLUMN, 6);
  for (const auto& slot : grid.slots) {
    printf("Grid slot %d: pos=(%.1f, %.1f) fwd=(%.2f, %.2f) w=%.1f d=%.1f\n",
           slot.slot_id,
           slot.transform.position.x(), slot.transform.position.y(),
           slot.transform.forward.x(), slot.transform.forward.y(),
           slot.width, slot.depth);
  }

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
