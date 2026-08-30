#include "tracking/replay_trajectory.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace p0::tracking {

ReplayTrajectory::ReplayTrajectory(std::vector<Sample> samples)
    : samples_(std::move(samples)) {}

PositionSample ReplayTrajectory::sample_at(double time) {
  PositionSample sample{};
  if (samples_.empty()) return sample;

  const int idx = find_sample_index(time);
  if (idx < 0) return sample;

  const Sample& s = samples_[static_cast<size_t>(idx)];
  sample.timestamp = s.timestamp;
  sample.latitude = s.latitude;
  sample.longitude = s.longitude;
  sample.altitude = s.altitude;
  sample.speed = s.speed;
  sample.heading = s.heading;
  sample.valid = s.valid;
  return sample;
}

double ReplayTrajectory::duration() const {
  if (samples_.empty()) return 0.0;
  return samples_.back().timestamp - samples_.front().timestamp;
}

int ReplayTrajectory::find_sample_index(double time) const {
  if (samples_.empty()) return -1;

  auto it = std::lower_bound(
      samples_.begin(), samples_.end(), time,
      [](const Sample& s, double t) { return s.timestamp < t; });

  if (it == samples_.end()) return static_cast<int>(samples_.size()) - 1;
  if (it == samples_.begin()) return 0;
  return static_cast<int>(it - samples_.begin());
}

ReplayTrajectory ReplayTrajectory::from_csv(const std::string& path) {
  std::vector<Sample> samples;
  std::ifstream file(path);
  if (!file.is_open()) return ReplayTrajectory{std::move(samples)};

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty()) continue;
    std::istringstream iss(line);
    Sample s{};
    char comma;
    if (!(iss >> s.timestamp >> comma >> s.latitude >> comma >>
          s.longitude >> comma >> s.altitude >> comma >>
          s.speed >> comma >> s.heading >> comma >> s.valid)) {
      continue;
    }
    samples.push_back(s);
  }

  return ReplayTrajectory{std::move(samples)};
}

}  // namespace p0::tracking
