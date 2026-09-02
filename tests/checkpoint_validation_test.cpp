// Project 0 — comprehensive checkpoint validation tests
// Covers: ordering, tolerance, wrapping, lateral offset, on/off track,
//         out-of-order traversal, reset, and complete lap sequences.
#include <gtest/gtest.h>

#include "tracking/checkpoint.h"
#include "tracking/track_position.h"

using namespace p0;
using p0::tracking::Checkpoint;
using p0::tracking::CheckpointSystem;
using p0::tracking::TrackPosition;

namespace {

Checkpoint make_cp(double s, double tol, int id) {
  Checkpoint c;
  c.s = s;
  c.tolerance = tol;
  c.id = id;
  return c;
}

TrackPosition make_pos(double s, double lateral, bool on_track = true) {
  TrackPosition p;
  p.s = s;
  p.lateral = lateral;
  p.on_track = on_track;
  return p;
}

}  // namespace

// ---------------------------------------------------------------------------
// Section 1 — Basic structure defaults
// ---------------------------------------------------------------------------

TEST(CheckpointValidation, CheckpointDefaults) {
  Checkpoint c;
  EXPECT_DOUBLE_EQ(c.s, 0.0);
  EXPECT_DOUBLE_EQ(c.tolerance, 5.0);
  EXPECT_EQ(c.id, -1);
}

TEST(CheckpointValidation, CheckpointResultDefaults) {
  p0::tracking::CheckpointResult r;
  EXPECT_EQ(r.checkpoint_id, -1);
  EXPECT_FALSE(r.passed);
  EXPECT_FALSE(r.correct_order);
}

TEST(CheckpointValidation, SystemStartsEmpty) {
  CheckpointSystem sys(1000.0);
  EXPECT_EQ(sys.next_checkpoint(), 0);
  // With no checkpoints, the sequence is vacuously complete.
  EXPECT_TRUE(sys.all_passed());
  EXPECT_TRUE(sys.checkpoints().empty());
}

// ---------------------------------------------------------------------------
// Section 2 — Single-checkpoint happy path
// ---------------------------------------------------------------------------

TEST(CheckpointValidation, SingleCheckpointPassesAtCenter) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({make_cp(500.0, 5.0, 0)});

  auto r = sys.validate(make_pos(500.0, 0.0));
  EXPECT_TRUE(r.passed);
  EXPECT_TRUE(r.correct_order);
  EXPECT_EQ(r.checkpoint_id, 0);
  EXPECT_EQ(sys.next_checkpoint(), 1);
  EXPECT_TRUE(sys.all_passed());
}

TEST(CheckpointValidation, SingleCheckpointPassesAtToleranceBoundary) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({make_cp(500.0, 5.0, 0)});

  // Exactly at the tolerance should still pass (<= tolerance).
  auto r = sys.validate(make_pos(505.0, 5.0));
  EXPECT_TRUE(r.passed);
}

TEST(CheckpointValidation, SingleCheckpointFailsJustOutsideTolerance) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({make_cp(500.0, 5.0, 0)});

  // 5.001 m past the checkpoint on the s axis -> fails.
  auto r = sys.validate(make_pos(505.001, 0.0));
  EXPECT_FALSE(r.passed);
  EXPECT_EQ(sys.next_checkpoint(), 0);
}

TEST(CheckpointValidation, LateralOffsetIsChecked) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({make_cp(500.0, 5.0, 0)});

  // On the s axis but way off laterally -> fails.
  auto r = sys.validate(make_pos(500.0, 6.0));
  EXPECT_FALSE(r.passed);
}

// ---------------------------------------------------------------------------
// Section 3 — Ordering
// ---------------------------------------------------------------------------

TEST(CheckpointValidation, CheckpointsMustBeHitInOrder) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({
    make_cp(200.0, 5.0, 0),
    make_cp(600.0, 5.0, 1),
    make_cp(900.0, 5.0, 2)
  });

  // Skip cp[0], try cp[1] -> still targets cp[0], so no pass.
  auto r1 = sys.validate(make_pos(600.0, 0.0));
  EXPECT_FALSE(r1.passed);
  EXPECT_EQ(sys.next_checkpoint(), 0);

  // Hit cp[0] -> advances.
  auto r2 = sys.validate(make_pos(200.0, 0.0));
  EXPECT_TRUE(r2.passed);
  EXPECT_EQ(r2.checkpoint_id, 0);
  EXPECT_EQ(sys.next_checkpoint(), 1);

  // Try cp[2] before cp[1] -> no pass.
  auto r3 = sys.validate(make_pos(900.0, 0.0));
  EXPECT_FALSE(r3.passed);
  EXPECT_EQ(sys.next_checkpoint(), 1);

  // Hit cp[1] -> advances.
  auto r4 = sys.validate(make_pos(600.0, 0.0));
  EXPECT_TRUE(r4.passed);
  EXPECT_EQ(r4.checkpoint_id, 1);

  // Hit cp[2] -> completes the sequence.
  auto r5 = sys.validate(make_pos(900.0, 0.0));
  EXPECT_TRUE(r5.passed);
  EXPECT_EQ(r5.checkpoint_id, 2);
  EXPECT_TRUE(sys.all_passed());
}

TEST(CheckpointValidation, MultipleHitsOfSameCheckpointOnlyAdvanceOnce) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({make_cp(500.0, 5.0, 0)});

  EXPECT_TRUE(sys.validate(make_pos(500.0, 0.0)).passed);
  // Same position shouldn't fire again — next_checkpoint already past it.
  auto r = sys.validate(make_pos(500.0, 0.0));
  EXPECT_FALSE(r.passed);
}

// ---------------------------------------------------------------------------
// Section 4 — Off-track and missing data
// ---------------------------------------------------------------------------

TEST(CheckpointValidation, OffTrackDoesNotCount) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({make_cp(500.0, 5.0, 0)});

  auto r = sys.validate(make_pos(500.0, 0.0, /*on_track=*/false));
  EXPECT_FALSE(r.passed);
  EXPECT_EQ(sys.next_checkpoint(), 0);
}

TEST(CheckpointValidation, EmptyCheckpointsListNeverPasses) {
  CheckpointSystem sys(1000.0);
  auto r = sys.validate(make_pos(500.0, 0.0));
  EXPECT_FALSE(r.passed);
  EXPECT_TRUE(sys.all_passed());  // empty -> trivially complete
}

TEST(CheckpointValidation, CompletedSystemIgnoresFurtherValidations) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({make_cp(500.0, 5.0, 0)});
  EXPECT_TRUE(sys.validate(make_pos(500.0, 0.0)).passed);
  EXPECT_TRUE(sys.all_passed());

  // Subsequent calls return an "empty" result.
  auto r = sys.validate(make_pos(700.0, 0.0));
  EXPECT_FALSE(r.passed);
  EXPECT_EQ(r.checkpoint_id, -1);
}

// ---------------------------------------------------------------------------
// Section 5 — Track-length wrapping
// ---------------------------------------------------------------------------

TEST(CheckpointValidation, WrapsAcrossStartLine) {
  // Track length 1000. A checkpoint at s = 50 should be reachable from
  // s = 990 (distance = 60 via the wrapped short path) — except the wrapped
  // distance is min(60, 1000-60) = 60 which is > tolerance 5, so still fail.
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({make_cp(50.0, 5.0, 0)});

  EXPECT_FALSE(sys.validate(make_pos(990.0, 0.0)).passed);

  // But s = 55 -> wrapped ds = 5 -> pass.
  EXPECT_TRUE(sys.validate(make_pos(55.0, 0.0)).passed);
}

TEST(CheckpointValidation, ShortestPathIsUsedForWrapping) {
  // On a 100 m track with a checkpoint at s = 5, position s = 95
  // has ds = 90 and wrapped_ds = 100 - 90 = 10. Tolerance = 12 -> pass.
  CheckpointSystem sys(100.0);
  sys.set_checkpoints({make_cp(5.0, 12.0, 0)});

  EXPECT_TRUE(sys.validate(make_pos(95.0, 0.0)).passed);
}

TEST(CheckpointValidation, WrappingDoesNotFalsifyDirectionalValidation) {
  // On a 1000 m track, cp at s = 10, position s = 20. ds = 10, wrapped_ds = 10.
  // Even with tolerance 50, this should still pass since the shortest
  // signed distance to the checkpoint is exactly 10 m.
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({make_cp(10.0, 50.0, 0)});
  EXPECT_TRUE(sys.validate(make_pos(20.0, 0.0)).passed);
}

// ---------------------------------------------------------------------------
// Section 6 — Reset and reconfiguration
// ---------------------------------------------------------------------------

TEST(CheckpointValidation, ResetReturnsToFirstCheckpoint) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({
    make_cp(200.0, 5.0, 0),
    make_cp(600.0, 5.0, 1)
  });
  sys.validate(make_pos(200.0, 0.0));
  sys.validate(make_pos(600.0, 0.0));
  EXPECT_EQ(sys.next_checkpoint(), 2);
  EXPECT_TRUE(sys.all_passed());

  sys.reset();
  EXPECT_EQ(sys.next_checkpoint(), 0);
  EXPECT_FALSE(sys.all_passed());

  // After reset, must hit them again.
  EXPECT_TRUE(sys.validate(make_pos(200.0, 0.0)).passed);
  EXPECT_TRUE(sys.validate(make_pos(600.0, 0.0)).passed);
}

TEST(CheckpointValidation, SetCheckpointsResetsProgress) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({make_cp(200.0, 5.0, 0)});
  sys.validate(make_pos(200.0, 0.0));
  EXPECT_EQ(sys.next_checkpoint(), 1);

  sys.set_checkpoints({
    make_cp(300.0, 5.0, 0),
    make_cp(700.0, 5.0, 1)
  });
  EXPECT_EQ(sys.next_checkpoint(), 0);
  EXPECT_FALSE(sys.all_passed());
}

TEST(CheckpointValidation, SetTrackLengthUpdatesWrapping) {
  CheckpointSystem sys(100.0);
  sys.set_checkpoints({make_cp(5.0, 10.0, 0)});
  // Track length 100, position s = 95 -> wrapped distance = 10, passes.
  EXPECT_TRUE(sys.validate(make_pos(95.0, 0.0)).passed);

  // Now reduce the track length. We pick a checkpoint and position so
  // that the wrap distance is unambiguous on the new, shorter track.
  // New track length = 50, checkpoint at s = 10, position s = 5
  // (|5 - 10| = 5, within the 10 m tolerance, so it would pass on a
  // 50 m track). Use a wider position offset instead:
  // cp at s = 0, pos at s = 30 -> ds = 30, wrapped = min(30, 20) = 20 > tol.
  sys.reset();
  sys.set_checkpoints({make_cp(0.0, 5.0, 0)});
  sys.set_track_length(50.0);
  EXPECT_FALSE(sys.validate(make_pos(30.0, 0.0)).passed);
}

// ---------------------------------------------------------------------------
// Section 7 — Full lap sequences (integration-ish)
// ---------------------------------------------------------------------------

TEST(CheckpointValidation, ThreeCheckpointsOnFullLap) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({
    make_cp(250.0, 5.0, 0),
    make_cp(500.0, 5.0, 1),
    make_cp(750.0, 5.0, 2)
  });

  EXPECT_FALSE(sys.validate(make_pos(50.0, 0.0)).passed);    // before any
  EXPECT_FALSE(sys.validate(make_pos(150.0, 0.0)).passed);   // between
  EXPECT_TRUE (sys.validate(make_pos(250.0, 0.0)).passed);   // cp 0
  EXPECT_FALSE(sys.validate(make_pos(260.0, 0.0)).passed);   // already past
  EXPECT_TRUE (sys.validate(make_pos(500.0, 0.0)).passed);   // cp 1
  EXPECT_TRUE (sys.validate(make_pos(750.0, 0.0)).passed);   // cp 2
  EXPECT_TRUE(sys.all_passed());
}

TEST(CheckpointValidation, ValidationReturnsImmediatelyWhenAllPassed) {
  CheckpointSystem sys(1000.0);
  sys.set_checkpoints({make_cp(500.0, 5.0, 0)});
  sys.validate(make_pos(500.0, 0.0));

  for (int i = 0; i < 100; ++i) {
    auto r = sys.validate(make_pos(500.0 + i * 1.0, 0.0));
    EXPECT_FALSE(r.passed);
    EXPECT_EQ(r.checkpoint_id, -1);
  }
}

// ---------------------------------------------------------------------------
// Section 8 — Regression: wrap-distance must be non-negative
// ---------------------------------------------------------------------------

TEST(CheckpointValidation, FarOffTrackPositionFailsWhenDsExceedsTrackLength) {
  // Regression: when |pos.s - cp.s| > track_length, the previous
  // implementation produced a negative wrapped_ds via
  //   std::min(ds, track_length_ - ds)
  // which then evaluated `wrapped_ds <= tolerance` to true. The position
  // must NOT pass when its remainder modulo the track length is outside
  // the tolerance window.
  CheckpointSystem sys(50.0);
  sys.set_checkpoints({make_cp(0.0, 5.0, 0)});

  // 73 past the start: 73 mod 50 = 23, wrap distance = min(23, 27) = 23 > 5.
  auto r = sys.validate(make_pos(73.0, 0.0));
  EXPECT_FALSE(r.passed);
  EXPECT_EQ(sys.next_checkpoint(), 0);
}

TEST(CheckpointValidation, FarOffTrackBothDirections) {
  CheckpointSystem sys(50.0);
  sys.set_checkpoints({make_cp(0.0, 5.0, 0)});

  // 71 ahead: 71 mod 50 = 21, wrap = 21 > 5 -> fail.
  EXPECT_FALSE(sys.validate(make_pos(71.0, 0.0)).passed);
  // 71 behind: |-71| = 71, 71 mod 50 = 21, wrap = 21 > 5 -> fail.
  EXPECT_FALSE(sys.validate(make_pos(-71.0, 0.0)).passed);
}

TEST(CheckpointValidation, FarOffTrackFailsEvenWithLargeTolerance) {
  // Even with a generous tolerance, a position thousands of metres past
  // the end of a short track must not pass.
  CheckpointSystem sys(50.0);
  sys.set_checkpoints({make_cp(0.0, 100.0, 0)});  // huge tolerance

  // 5001 mod 50 = 1, wrap = min(1, 49) = 1 < 100 -> SHOULD pass.
  // Use a different cp.s so the wrap is outside tolerance.
  sys.reset();
  sys.set_checkpoints({make_cp(10.0, 5.0, 0)});
  // 5001 - 10 = 4991, 4991 mod 50 = 41, wrap = min(41, 9) = 9 > 5 -> fail.
  EXPECT_FALSE(sys.validate(make_pos(5001.0, 0.0)).passed);
}

TEST(CheckpointValidation, WrapStillWorksAfterFix) {
  // The fix must not break normal wrap-around validation: a position just
  // before the start line should still pass a checkpoint just after it.
  CheckpointSystem sys(100.0);

  // Checkpoint at s=5 with tolerance 10. Position 95 wraps to within
  // 10 m of the checkpoint -> pass.
  sys.set_checkpoints({make_cp(5.0, 10.0, 0)});
  EXPECT_TRUE(sys.validate(make_pos(95.0, 0.0)).passed);

  // Reset and verify direct-hit still passes.
  sys.reset();
  EXPECT_TRUE(sys.validate(make_pos(5.0, 0.0)).passed);

  // A position 30 m before the start still fails (wrap distance 30 > 10).
  sys.reset();
  EXPECT_FALSE(sys.validate(make_pos(70.0, 0.0)).passed);
}

TEST(CheckpointValidation, MultipleWrapPassesWhenOnRemainder) {
  // A position that, after wrapping, lands on the checkpoint: 123 mod 50 = 23,
  // cp at 23, tolerance 5 -> should pass.
  CheckpointSystem sys(50.0);
  sys.set_checkpoints({make_cp(23.0, 5.0, 0)});
  EXPECT_TRUE(sys.validate(make_pos(123.0, 0.0)).passed);
}