#include <gtest/gtest.h>
#include "common.h"

TEST(Clamp, BelowMinReturnsMin) {
  EXPECT_DOUBLE_EQ(p0::clamp(-5.0, 0.0, 10.0), 0.0);
}

TEST(Clamp, AboveMaxReturnsMax) {
  EXPECT_DOUBLE_EQ(p0::clamp(15.0, 0.0, 10.0), 10.0);
}

TEST(Clamp, InsideRangeUnchanged) {
  EXPECT_DOUBLE_EQ(p0::clamp(5.0, 0.0, 10.0), 5.0);
}

TEST(NormalizeAngle, ZeroStaysZero) {
  EXPECT_DOUBLE_EQ(p0::normalize_angle(0.0), 0.0);
}

TEST(NormalizeAngle, PositiveWraps) {
  EXPECT_DOUBLE_EQ(p0::normalize_angle(p0::kPi + 0.5), -p0::kPi + 0.5);
}

TEST(NormalizeAngle, NegativeWraps) {
  EXPECT_DOUBLE_EQ(p0::normalize_angle(-p0::kPi - 0.5), p0::kPi - 0.5);
}

TEST(NormalizeAngle, NonFiniteReturnsZero) {
  EXPECT_DOUBLE_EQ(p0::normalize_angle(std::numeric_limits<double>::quiet_NaN()), 0.0);
  EXPECT_DOUBLE_EQ(p0::normalize_angle(std::numeric_limits<double>::infinity()), 0.0);
}

TEST(NormalizeAngle, MultipleRotations) {
  double a = 4.5 * p0::kPi;
  double n = p0::normalize_angle(a);
  EXPECT_GE(n, -p0::kPi);
  EXPECT_LT(n, p0::kPi);
}

TEST(LerpScalar, Midpoint) {
  EXPECT_DOUBLE_EQ(p0::lerp(0.0, 10.0, 0.5), 5.0);
}

TEST(LerpScalar, Endpoints) {
  EXPECT_DOUBLE_EQ(p0::lerp(2.0, 8.0, 0.0), 2.0);
  EXPECT_DOUBLE_EQ(p0::lerp(2.0, 8.0, 1.0), 8.0);
}

TEST(LerpVec2, Midpoint) {
  p0::Vec2 a(0.0, 0.0);
  p0::Vec2 b(4.0, 6.0);
  p0::Vec2 r = p0::lerp(a, b, 0.5);
  EXPECT_NEAR(r.x(), 2.0, 1e-9);
  EXPECT_NEAR(r.y(), 3.0, 1e-9);
}

TEST(Constants, PiValues) {
  EXPECT_DOUBLE_EQ(p0::kPi, 3.14159265358979323846);
  EXPECT_DOUBLE_EQ(p0::kTwoPi, 2.0 * p0::kPi);
  EXPECT_DOUBLE_EQ(p0::kHalfPi, 0.5 * p0::kPi);
  EXPECT_DOUBLE_EQ(p0::kDegToRad, p0::kPi / 180.0);
  EXPECT_DOUBLE_EQ(p0::kRadToDeg, 180.0 / p0::kPi);
  EXPECT_DOUBLE_EQ(p0::kGravity, 9.80665);
  EXPECT_DOUBLE_EQ(p0::kEpsilon, 1e-9);
}
