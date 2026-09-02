#include "tracking/checkpoint.h"
#include "tracking/track_position.h"

namespace p0::tracking {

CheckpointSystem::CheckpointSystem(double track_length)
    : track_length_(track_length) {}

void CheckpointSystem::set_checkpoints(const std::vector<Checkpoint>& checkpoints) {
  checkpoints_ = checkpoints;
  next_checkpoint_ = 0;
}

void CheckpointSystem::set_track_length(double length) {
  track_length_ = length;
}

CheckpointResult CheckpointSystem::validate(const TrackPosition& pos) {
  CheckpointResult result{};

  if (!pos.on_track || checkpoints_.empty()) return result;
  if (next_checkpoint_ >= static_cast<int>(checkpoints_.size())) return result;
  if (track_length_ <= 0.0) return result;

  const Checkpoint& cp = checkpoints_[static_cast<size_t>(next_checkpoint_)];

  const double ds = std::fabs(pos.s - cp.s);
  // Reduce ds into [0, track_length_] before computing the wrap distance.
  // Without this, when pos.s is far off the track (|delta| > track_length_)
  // the second term `track_length_ - ds` becomes negative, and std::min
  // returns that negative value, causing positions well outside the track
  // to incorrectly pass validation.
  double wrapped_ds = ds;
  if (track_length_ > 0.0) {
    wrapped_ds = std::fmod(ds, track_length_);
    wrapped_ds = std::min(wrapped_ds, track_length_ - wrapped_ds);
  }

  if (wrapped_ds <= cp.tolerance && std::fabs(pos.lateral) <= cp.tolerance) {
    result.checkpoint_id = cp.id;
    result.passed = true;
    result.correct_order = true;
    ++next_checkpoint_;
  }

  return result;
}

void CheckpointSystem::reset() {
  next_checkpoint_ = 0;
}

}  // namespace p0::tracking
