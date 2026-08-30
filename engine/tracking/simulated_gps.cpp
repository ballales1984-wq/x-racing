#include "tracking/simulated_gps.h"

#include <random>
#include <cmath>

namespace p0::tracking {

SimulatedGPS::SimulatedGPS(
    std::unique_ptr<ITrajectorySource> trajectory,
    Params params)
    : trajectory_(std::move(trajectory)),
      params_(params) {
  dt_per_sample_ = 1.0 / params.update_rate_hz;
}

bool SimulatedGPS::start() {
  if (!trajectory_) return false;
  running_ = true;
  last_sample_time_ = 0.0;
  return true;
}

void SimulatedGPS::stop() { running_ = false; }

bool SimulatedGPS::is_running() const { return running_; }

bool SimulatedGPS::poll(PositionSample& sample) {
  if (!running_ || !trajectory_) return false;

  last_sample_time_ = sample_time_advance();
  current_ = trajectory_->sample_at(last_sample_time_);
  apply_noise(current_);
  sample = current_;
  return true;
}

double SimulatedGPS::update_rate_hz() const { return params_.update_rate_hz; }

void SimulatedGPS::set_trajectory(std::unique_ptr<ITrajectorySource> trajectory) {
  trajectory_ = std::move(trajectory);
}

double SimulatedGPS::sample_time_advance() {
  if (current_.valid) {
    return current_.timestamp + dt_per_sample_;
  }
  return 0.0;
}

void SimulatedGPS::apply_noise(PositionSample& sample) {
  if (params_.noise_m <= 0.0) return;

  static thread_local std::mt19937 rng{std::random_device{}()};
  std::normal_distribution<double> dist(0.0, params_.noise_m);

  const double dx = dist(rng);
  const double dy = dist(rng);

  const double lat_rad = sample.latitude * kDegToRad;
  const double meters_per_deg_lat = 111132.92;
  const double meters_per_deg_lon = 111412.84 * std::cos(lat_rad);

  sample.latitude += (dy / meters_per_deg_lat) / kDegToRad;
  sample.longitude += (dx / meters_per_deg_lon) / kDegToRad;

  sample.horizontal_accuracy = params_.noise_m;

  if (params_.apply_heading_noise && params_.heading_noise_rad > 0.0) {
    std::normal_distribution<double> hdist(0.0, params_.heading_noise_rad);
    sample.heading += hdist(rng);
  }
}

}  // namespace p0::tracking
