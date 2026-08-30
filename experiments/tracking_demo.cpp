#include "tracking/tracking_system.h"
#include "tracking/lap_system.h"
#include "tracking/surface_system.h"
#include "tracking/tracked_telemetry.h"
#include "tracking/physics_trajectory.h"
#include "tracking/coordinate_converter.h"
#include "tracking/simulated_gps.h"
#include "track/track.h"
#include "vehicle/vehicle.h"

#include <iostream>
#include <memory>

int main() {
  using namespace p0::tracking;

  std::cout << "=== X-Racing Track Telemetry Demo ===\n\n";

  p0::track::Track track(p0::track::TrackType::Default);
  std::cout << "Track length: " << track.length() << " m\n";

  CoordinateConverter converter(GeographicOrigin{45.0, 11.0, 0.0});

  p0::vehicle::VehicleState state{};
  state.position = p0::Vec2(0.0, 0.0);
  state.heading = 0.0;
  state.velocity = p0::Vec2(30.0, 0.0);
  state.speed = 30.0;

  auto gps = std::make_unique<SimulatedGPS>(
      std::make_unique<PhysicsTrajectory>(converter, &state),
      SimulatedGPS::Params{10.0, 0.5});

  TrackingSystem tracking(std::move(gps));
  auto mapper = std::make_unique<TrackMapper>(track, TrackMapper::LocalOrigin{45.0, 11.0});
  tracking.set_mapper(std::move(mapper));

  LapSystem lap_system(track.length(), 3);
  SurfaceSystem surface(track);
  TrackedTelemetry telemetry("demo_telemetry.csv");

  tracking.start();

  std::cout << "Simulating 10 seconds of driving...\n";

  PositionSample gps_sample{};
  for (int i = 0; i < 100; ++i) {
    state.position += state.velocity * 0.1;
    state.heading += 0.02;

    tracking.update();

    const auto& pos = tracking.current_track_position();
    if (pos.on_track) {
      lap_system.update(pos, i * 0.1);
      surface.apply_vehicle({pos, state.speed, 1.0, true});

      if (tracking.current_sample().valid) {
        telemetry.record(pos, tracking.current_sample(),
                         surface.grip_at(pos),
                         surface.grid().cell(pos.s, pos.lateral).temperature,
                         surface.grid().cell(pos.s, pos.lateral).rubber,
                         lap_system);
      }
    }
  }

  surface.update(10.0);

  std::cout << "Laps completed: " << lap_system.completed_laps() << "\n";
  std::cout << "Last lap time: " << lap_system.last_lap_time() << " s\n";

  const auto& frames = telemetry.frames();
  std::cout << "Tracked frames: " << frames.size() << "\n";

  telemetry.save_csv();
  std::cout << "Telemetry saved to demo_telemetry.csv\n";

  return 0;
}
