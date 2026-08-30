#include "tracking/replay_gps.h"

namespace p0::tracking {

ReplayGPS::ReplayGPS(std::unique_ptr<ITrajectorySource> source, double rate_hz)
    : source_(std::move(source)), rate_hz_(rate_hz) {}

bool ReplayGPS::start() {
  if (!source_) return false;
  running_ = true;
  last_time_ = -1.0;
  return true;
}

void ReplayGPS::stop() { running_ = false; }

bool ReplayGPS::is_running() const { return running_; }

bool ReplayGPS::poll(PositionSample& sample) {
  if (!running_ || !source_) return false;
  last_time_ = advance_time_();
  sample = source_->sample_at(last_time_);
  return true;
}

double ReplayGPS::update_rate_hz() const { return rate_hz_; }

void ReplayGPS::set_source(std::unique_ptr<ITrajectorySource> source) {
  source_ = std::move(source);
}

double ReplayGPS::advance_time_() {
  if (last_time_ < 0.0) return 0.0;
  return last_time_ + 1.0 / rate_hz_;
}

}  // namespace p0::tracking
