#include "tracking/tracking_system.h"

namespace p0::tracking {

TrackingSystem::TrackingSystem(
    std::unique_ptr<IPositionProvider> provider,
    const IClock* clock)
    : provider_(std::move(provider)), clock_(clock) {}

bool TrackingSystem::start() {
  if (!provider_ || !provider_->start()) return false;
  running_ = true;
  return true;
}

void TrackingSystem::stop() {
  if (provider_) provider_->stop();
  running_ = false;
}

void TrackingSystem::update() {
  if (!running_) return;

  if (provider_->poll(current_sample_)) {
    if (mapper_) {
      current_track_ = mapper_->map(current_sample_);
    }
  }
}

void TrackingSystem::set_mapper(std::unique_ptr<TrackMapper> mapper) {
  mapper_ = std::move(mapper);
}

}  // namespace p0::tracking
