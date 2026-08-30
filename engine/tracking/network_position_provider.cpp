#include "tracking/network_position_provider.h"

namespace p0::tracking {

NetworkPositionProvider::NetworkPositionProvider(Params params)
    : params_(params) {
  if (params_.update_rate_hz <= 0.0) {
    params_.update_rate_hz = 10.0;
  }
}

bool NetworkPositionProvider::start() {
  running_ = true;
  has_sample_ = false;
  return true;
}

void NetworkPositionProvider::stop() { running_ = false; }

bool NetworkPositionProvider::is_running() const { return running_; }

bool NetworkPositionProvider::poll(PositionSample& sample) {
  if (!running_ || !has_sample_) return false;
  sample = latest_sample_;
  return true;
}

double NetworkPositionProvider::update_rate_hz() const { return params_.update_rate_hz; }

void NetworkPositionProvider::set_sample(PositionSample sample) {
  latest_sample_ = std::move(sample);
  has_sample_ = true;
}

}  // namespace p0::tracking
