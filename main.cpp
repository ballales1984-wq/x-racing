#include "renderer/renderer.h"
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

  renderer::Renderer renderer(sim);
  renderer.run();

  return 0;
}
