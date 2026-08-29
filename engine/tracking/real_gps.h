#pragma once

#include "common.h"
#include "tracking/position_provider.h"

// Project 0 — tracking / real GPS provider
// Namespace: p0::tracking
namespace p0::tracking {

// Real GPS provider wrapping an IGPSDevice transport.
// Keeps provider logic independent of serial/USB/BT/network details.
class RealGPS : public IPositionProvider {
 public:
  explicit RealGPS(std::unique_ptr<IGPSDevice> device);
  ~RealGPS() override = default;

  bool start() override;
  void stop() override;
  bool is_running() const override;
  bool update(PositionSample& sample) override;
  double update_rate_hz() const override;

  IGPSDevice* device() const { return device_.get(); }

 private:
  std::unique_ptr<IGPSDevice> device_;
  bool running_ = false;
};

}  // namespace p0::tracking
