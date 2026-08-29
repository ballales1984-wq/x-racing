#pragma once

#include "common.h"
#include <string>

namespace p0::debug {

struct ProfilerSnapshot {
  double fps = 0.0;
  double last_frame_time_ms = 0.0;
  double last_physics_time_ms = 0.0;
  double avg_frame_time_ms = 0.0;
  double max_frame_time_ms = 0.0;
  double min_frame_time_ms = 9999.0;
  double total_time_s = 0.0;
  int total_frames = 0;
  int dropped_frames = 0;
  double physics_ratio = 0.0;
};

class DebugProfiler {
 public:
  DebugProfiler() = default;

  void initialize();
  void shutdown();

  void begin_frame(double real_dt);
  void end_frame();
  void record_physics_step(double dt);

  const ProfilerSnapshot& snapshot() const { return snapshot_; }
  void reset();

 private:
  ProfilerSnapshot snapshot_;
  double frame_start_real_ = 0.0;
  double physics_time_accum_ = 0.0;
  double frame_time_accum_ = 0.0;
  double max_frame_accum_ = 0.0;
  double min_frame_accum_ = 9999.0;
  int frame_count_for_avg_ = 0;
  bool frame_in_progress_ = false;
};

}
