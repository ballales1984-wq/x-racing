#include "tracking/real_gps.h"

namespace p0::tracking {

RealGPS::RealGPS(std::unique_ptr<IGPSDevice> device)
    : device_(std::move(device)) {}

bool RealGPS::start() {
  if (!device_ || !device_->connect()) return false;
  running_ = true;
  return true;
}

void RealGPS::stop() {
  if (device_) device_->disconnect();
  running_ = false;
}

bool RealGPS::is_running() const { return running_; }

bool RealGPS::poll(PositionSample& sample) {
  if (!running_ || !device_) return false;
  return device_->read(sample);
}

double RealGPS::update_rate_hz() const {
  // Unknown until the underlying device reports its update frequency.
  return 0.0;
}

}  // namespace p0::tracking
