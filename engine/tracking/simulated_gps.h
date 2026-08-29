#pragma once

#include "common.h"
#include "tracking/position_provider.h"
#include "tracking/clock.h"

// Project 0 — tracking / simulated GPS provider
// Namespace: p0::tracking
namespace p0::tracking {

// Simulated GPS fed by an ITrajectorySource with optional noise.
// Useful for physics-driven simulation or GPX/replay replay.
class SimulatedGPS : public IPositionProvider {
 public:
  struct Params {
    double update_rate_hz = 10.0;
    double noise_m = 0.0;       // 1-sigma Gaussian noise applied to position
    bool apply_heading_noise = false;
    double heading_noise_rad = 0.0;
  };

  explicit SimulatedGPS(
      std::unique_ptr<ITrajectorySource> trajectory,
      Params params = {});

  ~SimulatedGPS() override = default;

  bool start() override;
  void stop() override;
  bool is_running() const override;
  bool update(PositionSample& sample) override;
  double update_rate_hz() const override;

  void set_trajectory(std::unique_ptr<ITrajectorySource> trajectory);

 private:
  double sample_time_advance();
  void apply_noise(PositionSample& sample);

  std::unique_ptr<ITrajectorySource> trajectory_;
  Params params_;

  bool running_ = false;
  double last_sample_time_ = 0.0;
  double dt_per_sample_ = 0.0;

  PositionSample current_;
};

}  // namespace p0::tracking
