#include "ai/racing_line.h"
#include <algorithm>
#include <cmath>

namespace p0::ai {

//! @brief Constructs the racing line optimizer and computes the optimal line.
//! @param track Reference to the track data.
RacingLineOptimizer::RacingLineOptimizer(const track::Track& track) : track_(&track) {
  compute();
}

//! @brief Computes the optimal racing line by calculating lateral offsets
//!        based on curvature and smoothing the result.
void RacingLineOptimizer::compute() {
  if (!track_ || track_->sample_count() < 2) return;

  int n = track_->sample_count();
  points_.reserve(n);

  for (int i = 0; i < n; ++i) {
    const auto& tp = track_->sample_at(i);
    RacingLinePoint rp;
    rp.position = tp.position;
    rp.tangent = tp.tangent;
    rp.curvature = tp.curvature;
    rp.distance = tp.distance;
    rp.lateral_offset = optimal_offset(tp.curvature, tp.width);
    rp.speed_m_s = apex_speed(std::abs(tp.curvature), tp.friction);

    rp.position += tp.normal * rp.lateral_offset;

    points_.push_back(rp);
  }

  std::vector<double> smooth(n);
  int window = 5;
  for (int i = 0; i < n; ++i) {
    double sum = 0.0;
    int count = 0;
    for (int j = -window; j <= window; ++j) {
      int idx = (i + j + n) % n;
      sum += points_[idx].lateral_offset;
      ++count;
    }
    smooth[i] = sum / count;
  }
  for (int i = 0; i < n; ++i) {
    points_[i].lateral_offset = smooth[i];
    const auto& tp = track_->sample_at(i);
    points_[i].position = tp.position + tp.normal * smooth[i];
  }
}

//! @brief Calculates the optimal lateral offset for a given curvature.
//!        Higher curvature results in larger offset to straighten the line.
//! @param curvature Track curvature at the point.
//! @param width Track width at the point.
//! @return Lateral offset from centerline (positive = right, negative = left).
double RacingLineOptimizer::optimal_offset(double curvature, double width) const {
  double half_width = width * 0.5 - 0.3;
  half_width = std::max(half_width, 0.5);

  double abs_c = std::abs(curvature);
  if (abs_c < 0.001) return 0.0;

  double sign = curvature > 0.0 ? 1.0 : -1.0;
  double offset = sign * half_width * std::clamp(abs_c * 200.0, 0.0, 1.0);

  return std::clamp(offset, -half_width, half_width);
}

//! @brief Calculates the maximum safe speed for a given curvature.
//!        Uses the friction circle model: v = sqrt(radius * g * mu).
//! @param curvature Track curvature (1/radius).
//! @param friction Surface friction coefficient.
//! @return Maximum speed in m/s.
double RacingLineOptimizer::apex_speed(double curvature, double friction) const {
  if (curvature < 0.0001) return 150.0;

  double radius = 1.0 / curvature;
  double g = 9.81;
  double mu = 1.8 * friction;
  double v = std::sqrt(radius * g * mu);
  v *= 0.92;

  return std::clamp(v, 30.0, 150.0);
}

//! @brief Converts racing line points to track racing line samples.
//! @return Vector of RacingLineSample for track integration.
std::vector<track::RacingLineSample> RacingLineOptimizer::to_racing_line_samples() const {
  std::vector<track::RacingLineSample> samples;
  samples.reserve(points_.size());

  for (const auto& rp : points_) {
    track::RacingLineSample s;
    s.transform.position = rp.position;
    s.transform.forward = rp.tangent;
    s.speed_m_s = rp.speed_m_s;
    s.throttle = 0.8;
    s.brake = 0.0;
    s.gear = 3.0;
    samples.push_back(s);
  }

  return samples;
}

//! @brief Returns the target speed at a given distance along the racing line.
//!        Uses linear interpolation between computed points.
//! @param distance Distance along the track centerline.
//! @return Target speed in m/s.
double RacingLineOptimizer::target_speed_at(double distance) const {
  if (points_.empty()) return 80.0;

  double track_len = track_->length();
  distance = std::fmod(distance, track_len);
  if (distance < 0.0) distance += track_len;

  auto it = std::lower_bound(points_.begin(), points_.end(), distance,
    [](const RacingLinePoint& rp, double d) { return rp.distance < d; });

  if (it == points_.end()) return points_.back().speed_m_s;
  if (it == points_.begin()) return points_.front().speed_m_s;

  const auto& prev = *(it - 1);
  const auto& curr = *it;
  double t = (distance - prev.distance) / (curr.distance - prev.distance + 1e-9);
  t = std::clamp(t, 0.0, 1.0);

  return prev.speed_m_s + t * (curr.speed_m_s - prev.speed_m_s);
}

}