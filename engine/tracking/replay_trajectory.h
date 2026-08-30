#pragma once

#include "common.h"
#include "tracking/position_provider.h"
#include <vector>
#include <string>

// Project 0 — tracking / replay trajectory source
// Namespace: p0::tracking
namespace p0::tracking {

// ITrajectorySource backed by a pre-recorded sequence of PositionSamples.
// Useful for replaying a recorded session, GPX file, or ghost car data.
class ReplayTrajectory : public ITrajectorySource {
 public:
  struct Sample {
    double timestamp = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    double speed = 0.0;
    double heading = 0.0;
    bool valid = false;
  };

  explicit ReplayTrajectory(std::vector<Sample> samples);

  PositionSample sample_at(double time) override;
  double duration() const override;

  static ReplayTrajectory from_csv(const std::string& path);

 private:
  int find_sample_index(double time) const;

  std::vector<Sample> samples_;
};

}  // namespace p0::tracking
