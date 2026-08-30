#pragma once

#include "common.h"
#include "tracking/position_provider.h"

// Project 0 — tracking / network position provider
// Namespace: p0::tracking
namespace p0::tracking {

// Placeholder network position provider for multiplayer remote positions.
// In a future iteration this will consume network-updated VehicleState
// snapshots from other players and expose them as PositionSamples.
class NetworkPositionProvider : public IPositionProvider {
 public:
  struct Params {
    double update_rate_hz;
  };

  explicit NetworkPositionProvider(Params params = {});
  ~NetworkPositionProvider() override = default;

  bool start() override;
  void stop() override;
  bool is_running() const override;
  bool poll(PositionSample& sample) override;
  double update_rate_hz() const override;

  void set_sample(PositionSample sample);

 private:
  Params params_;
  bool running_ = false;
  PositionSample latest_sample_;
  bool has_sample_ = false;
};

}  // namespace p0::tracking
