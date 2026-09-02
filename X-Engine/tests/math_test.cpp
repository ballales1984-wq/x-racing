#include <gtest/gtest.h>
#include "core/math.h"

TEST(MathTest, IdentityMultiplication) {
    auto a = xe::Mat4::Identity();
    auto b = xe::Mat4::Translation(2.0f, 3.0f, 4.0f);
    auto r = xe::Mat4::Multiply(a, b);
    EXPECT_FLOAT_EQ(r.m[0][3], 2.0f);
    EXPECT_FLOAT_EQ(r.m[1][3], 3.0f);
    EXPECT_FLOAT_EQ(r.m[2][3], 4.0f);
    EXPECT_FLOAT_EQ(r.m[3][3], 1.0f);
}

TEST(MathTest, RotationZZeroIsIdentity) {
    auto r = xe::Mat4::RotationZ(0.0f);
    EXPECT_FLOAT_EQ(r.m[0][0], 1.0f);
    EXPECT_FLOAT_EQ(r.m[1][1], 1.0f);
    EXPECT_FLOAT_EQ(r.m[0][1], 0.0f);
    EXPECT_FLOAT_EQ(r.m[1][0], 0.0f);
}

TEST(MathTest, RotationZQuarterTurn) {
    auto r = xe::Mat4::RotationZ(1.5707963f);
    EXPECT_NEAR(r.m[0][0],  0.0f, 1e-5f);
    EXPECT_NEAR(r.m[1][1],  0.0f, 1e-5f);
    EXPECT_NEAR(r.m[0][1], -1.0f, 1e-5f);
    EXPECT_NEAR(r.m[1][0],  1.0f, 1e-5f);
}

TEST(MathTest, OrthographicProjectsPoint) {
    auto p = xe::Mat4::Orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(p.m[0][0], 1.0f);
    EXPECT_FLOAT_EQ(p.m[1][1], 1.0f);
    EXPECT_FLOAT_EQ(p.m[0][3], 0.0f);
    EXPECT_FLOAT_EQ(p.m[1][3], 0.0f);
}

TEST(MathTest, ScaleScalesDiagonal) {
    auto s = xe::Mat4::Scale(2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(s.m[0][0], 2.0f);
    EXPECT_FLOAT_EQ(s.m[1][1], 3.0f);
    EXPECT_FLOAT_EQ(s.m[2][2], 4.0f);
    EXPECT_FLOAT_EQ(s.m[3][3], 1.0f);
}