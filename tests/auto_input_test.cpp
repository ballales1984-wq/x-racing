// Project 0 — unit tests for the scripted AutoInputManager
// Verifies the deterministic time-based input sequence.
#include <gtest/gtest.h>
#include "input/platform/auto_input.h"

using namespace p0::input;

// Helper: poll `count` times (advancing the scripted clock by count/60 s).
static void Advance(AutoInputManager& mgr, int count) {
  for (int i = 0; i < count; ++i) mgr.poll();
}

// The 0-3s window is full throttle on a straight line.
TEST(AutoInput, AcceleratesOnStraight) {
  AutoInputManager mgr;
  InputState s = mgr.poll();  // t = 0
  EXPECT_DOUBLE_EQ(s.throttle, 1.0);
  EXPECT_DOUBLE_EQ(s.steering, 0.0);
  EXPECT_DOUBLE_EQ(s.brake, 0.0);
  EXPECT_FALSE(s.reset);

  Advance(mgr, 90);  // advance to ~1.5s, well within the straight window
  for (int i = 0; i < 50; ++i) {
    InputState st = mgr.poll();
    EXPECT_DOUBLE_EQ(st.throttle, 1.0);
    EXPECT_DOUBLE_EQ(st.steering, 0.0);
  }
}

// The 3-8s window is a full-throttle right-hand curve (target t = 5s).
TEST(AutoInput, RightCurve) {
  AutoInputManager mgr;
  Advance(mgr, 300);  // advance to ~5s
  InputState s = mgr.poll();
  EXPECT_DOUBLE_EQ(s.throttle, 1.0);
  EXPECT_DOUBLE_EQ(s.steering, 1.0);

  for (int i = 0; i < 50; ++i) {
    InputState st = mgr.poll();
    EXPECT_DOUBLE_EQ(st.steering, 1.0);
  }
}

// The 8-13s window is a full-throttle left-hand curve (target t = 10s).
TEST(AutoInput, LeftCurve) {
  AutoInputManager mgr;
  Advance(mgr, 600);  // advance to ~10s
  InputState s = mgr.poll();
  EXPECT_DOUBLE_EQ(s.throttle, 1.0);
  EXPECT_DOUBLE_EQ(s.steering, -1.0);

  for (int i = 0; i < 50; ++i) {
    InputState st = mgr.poll();
    EXPECT_DOUBLE_EQ(st.steering, -1.0);
  }
}

// The 13-16s window is a braking zone (target t = 14s).
TEST(AutoInput, BrakingZone) {
  AutoInputManager mgr;
  Advance(mgr, 840);  // advance to ~14s
  InputState s = mgr.poll();
  EXPECT_DOUBLE_EQ(s.throttle, 0.0);
  EXPECT_DOUBLE_EQ(s.brake, 1.0);
  EXPECT_FALSE(s.reset);

  for (int i = 0; i < 50; ++i) {
    InputState st = mgr.poll();
    EXPECT_DOUBLE_EQ(st.brake, 1.0);
  }
}

// After 16s the sequence resets and restarts from the beginning.
TEST(AutoInput, SequenceResetsAndRestarts) {
  AutoInputManager mgr;
  Advance(mgr, 900);  // advance to ~15s (braking zone), just before the loop boundary

  bool saw_reset = false;
  for (int i = 0; i < 120 && !saw_reset; ++i) {
    InputState st = mgr.poll();
    if (st.reset) {
      saw_reset = true;
      // The very next poll should restart the sequence at t = 0 (straight).
      InputState restarted = mgr.poll();
      EXPECT_DOUBLE_EQ(restarted.throttle, 1.0);
      EXPECT_DOUBLE_EQ(restarted.steering, 0.0);
      EXPECT_FALSE(restarted.reset);
    }
  }
  EXPECT_TRUE(saw_reset);
}

// AutoInputManager never reports a key as pressed.
TEST(AutoInput, NoKeysDown) {
  AutoInputManager mgr;
  EXPECT_FALSE(mgr.is_key_down(0));
  EXPECT_FALSE(mgr.is_key_down(65));
}
