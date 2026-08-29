#include <gtest/gtest.h>
#include "track/lap_detector.h"

using namespace p0::track;

// A forward crossing of the start/finish line increments the lap count once.
TEST(LapDetector, ForwardCrossingCountsLap) {
  LapDetector detector(100.0);
  bool crossed = false;
  // Drive forward up to just before the line, then wrap past it.
  for (double d = 0.0; d < 96.0; d += 4.0) crossed = detector.update(d);
  // Wrap from ~96 to ~4 (forward crossing).
  crossed = detector.update(4.0);
  EXPECT_TRUE(crossed);
  EXPECT_EQ(detector.completed_laps(), 1);
}

// Crossing the line in reverse must NOT count a lap.
TEST(LapDetector, BackwardCrossingDoesNotCount) {
  LapDetector detector(100.0);
  // Drive forward and wrap across the line to count lap 1.
  for (double d = 0.0; d < 96.0; d += 4.0) detector.update(d);
  bool crossed = detector.update(4.0);  // forward wrap: 96 -> 4
  EXPECT_TRUE(crossed);
  EXPECT_EQ(detector.completed_laps(), 1);

  // Now move backwards across the line (wrap from ~4 to ~96).
  crossed = detector.update(96.0);
  EXPECT_FALSE(crossed);
  EXPECT_EQ(detector.completed_laps(), 0);
}

// Multiple forward laps are counted correctly.
TEST(LapDetector, MultipleLaps) {
  LapDetector detector(100.0);
  int laps = 0;
  for (int i = 0; i < 20; ++i) {
    for (double d = 0.0; d < 96.0; d += 4.0) detector.update(d);
    if (detector.update(4.0)) ++laps;
  }
  EXPECT_EQ(laps, 20);
  EXPECT_EQ(detector.completed_laps(), 20);
}

// finished() reflects the configured total lap count.
TEST(LapDetector, FinishedFlag) {
  LapDetector detector(100.0, 3);
  EXPECT_FALSE(detector.finished());
  for (int lap = 0; lap < 3; ++lap) {
    for (double d = 0.0; d < 96.0; d += 4.0) detector.update(d);
    detector.update(4.0);
  }
  EXPECT_TRUE(detector.finished());
}

// reset() re-arms the detector without counting a bogus lap.
TEST(LapDetector, ResetClearsLaps) {
  LapDetector detector(100.0);
  for (double d = 0.0; d < 96.0; d += 4.0) detector.update(d);
  detector.update(2.0);  // lap 1
  EXPECT_EQ(detector.completed_laps(), 1);
  detector.reset(0.0);
  EXPECT_EQ(detector.completed_laps(), 0);
  // A near-zero delta after reset must not count as a crossing.
  detector.update(1.0);
  EXPECT_EQ(detector.completed_laps(), 0);
}
