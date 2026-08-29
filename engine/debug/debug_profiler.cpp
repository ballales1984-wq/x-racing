#include "debug/debug_profiler.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace p0::debug {

void DebugProfiler::initialize() {
  reset();
}

void DebugProfiler::shutdown() {
  reset();
}

void DebugProfiler::reset() {
  snapshot_ = ProfilerSnapshot{};
  frame_start_real_ = 0.0;
  physics_time_accum_ = 0.0;
  frame_time_accum_ = 0.0;
  max_frame_accum_ = 0.0;
  min_frame_accum_ = 9999.0;
  frame_count_for_avg_ = 0;
  frame_in_progress_ = false;
}

void DebugProfiler::begin_frame(double real_dt) {
  frame_start_real_ = real_dt;
  physics_time_accum_ = 0.0;
  frame_in_progress_ = true;
}

void DebugProfiler::end_frame() {
  if (!frame_in_progress_) return;

  const double frame_time_ms = frame_start_real_ * 1000.0;
  snapshot_.last_frame_time_ms = frame_time_ms;
  snapshot_.fps = frame_time_ms > 0.001 ? 1000.0 / frame_time_ms : 0.0;

  frame_time_accum_ += frame_time_ms;
  max_frame_accum_ = std::max(max_frame_accum_, frame_time_ms);
  min_frame_accum_ = std::min(min_frame_accum_, frame_time_ms);
  frame_count_for_avg_++;

  if (frame_time_ms > 33.0) {
    snapshot_.dropped_frames++;
  }

  if (frame_count_for_avg_ > 0) {
    snapshot_.avg_frame_time_ms = frame_time_accum_ / frame_count_for_avg_;
    snapshot_.max_frame_time_ms = max_frame_accum_;
    snapshot_.min_frame_time_ms = min_frame_accum_;
  }

  snapshot_.total_frames++;
  snapshot_.total_time_s += frame_start_real_;

  const double phys_ms = physics_time_accum_ * 1000.0;
  snapshot_.last_physics_time_ms = phys_ms;
  snapshot_.physics_ratio = frame_time_ms > 0.001 ? phys_ms / frame_time_ms : 0.0;

  frame_in_progress_ = false;

  if (frame_count_for_avg_ > 600) {
    frame_time_accum_ /= 2.0;
    max_frame_accum_ /= 2.0;
    min_frame_accum_ /= 2.0;
    frame_count_for_avg_ /= 2;
  }
}

void DebugProfiler::record_physics_step(double dt) {
  physics_time_accum_ += dt;
}

}
