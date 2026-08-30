#pragma once

#include "common.h"
#include "tracking/track_position.h"

// Project 0 — tracking / checkpoint system
// Namespace: p0::tracking
namespace p0::tracking {

// A checkpoint is a point along the track that must be passed
// in the correct order for a lap to be valid.
struct Checkpoint {
  double s = 0.0;              // m, position along track centerline
  double tolerance = 5.0;      // m, maximum deviation from centerline
  int id = -1;                 // unique identifier
};

// Result of a checkpoint validation.
struct CheckpointResult {
  int checkpoint_id = -1;
  bool passed = false;
  bool correct_order = false;
};

// Manages a sequence of checkpoints and validates passage.
class CheckpointSystem {
 public:
  explicit CheckpointSystem(double track_length = 0.0);

  void set_checkpoints(const std::vector<Checkpoint>& checkpoints);
  void set_track_length(double length);

  CheckpointResult validate(const TrackPosition& pos);

  void reset();
  int next_checkpoint() const { return next_checkpoint_; }
  bool all_passed() const { return next_checkpoint_ >= static_cast<int>(checkpoints_.size()); }
  const std::vector<Checkpoint>& checkpoints() const { return checkpoints_; }

 private:
  double track_length_;
  std::vector<Checkpoint> checkpoints_;
  int next_checkpoint_ = 0;
};

}  // namespace p0::tracking
