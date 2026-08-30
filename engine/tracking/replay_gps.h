#pragma once

#include "common.h"
#include "tracking/position_provider.h"

// Project 0 — tracking / replay GPS provider
// Namespace: p0::tracking
namespace p0::tracking {

// Replay GPS fed by an ITrajectorySource (e.g. a recorded session or a
// synthetic path). Produces deterministic PositionSamples at a fixed rate,
// independent of the live physics engine.
//
//   ITrajectorySource -> PositionSample
class ReplayGPS : public IPositionProvider {
 public:
  explicit ReplayGPS(std::unique_ptr<ITrajectorySource> source,
                     double rate_hz = 10.0);
  ~ReplayGPS() override = default;

  bool start() override;
  void stop() override;
  bool is_running() const override;
  bool poll(PositionSample& sample) override;
  double update_rate_hz() const override;

  void set_source(std::unique_ptr<ITrajectorySource> source);

 private:
  double advance_time_();

  std::unique_ptr<ITrajectorySource> source_;
  double rate_hz_;
  double last_time_ = -1.0;
  bool running_ = false;
};

}  // namespace p0::tracking
