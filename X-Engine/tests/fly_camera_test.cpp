#include <gtest/gtest.h>
#include "core/fly_camera.h"

TEST(FlyCameraTest, InitialPosition) {
    xe::FlyCameraController c;
    float x, y, z;
    c.GetPosition(x, y, z);
    EXPECT_FLOAT_EQ(x, 0.0f);
    EXPECT_FLOAT_EQ(y, 1.5f);
    EXPECT_FLOAT_EQ(z, -4.0f);
}

TEST(FlyCameraTest, ForwardAtZeroYaw) {
    xe::FlyCameraController c;
    c.SetYaw(0.0f);
    c.SetPitch(0.0f);
    auto f = c.GetForward();
    // yaw=0, pitch=0 → forward = (sin(0), sin(0), -cos(0)) = (0, 0, -1)
    EXPECT_NEAR(f.x, 0.0f, 1e-5f);
    EXPECT_NEAR(f.y, 0.0f, 1e-5f);
    EXPECT_NEAR(f.z, -1.0f, 1e-5f);
}

TEST(FlyCameraTest, ForwardAtQuarterYaw) {
    xe::FlyCameraController c;
    c.SetYaw(1.5707963f);  // 90deg
    c.SetPitch(0.0f);
    auto f = c.GetForward();
    EXPECT_NEAR(f.x, 1.0f, 1e-4f);
    EXPECT_NEAR(f.z, 0.0f, 1e-4f);
}

TEST(FlyCameraTest, PitchClamped) {
    xe::FlyCameraController c;
    c.Update(0.016f, false, false, false, false, false, false, 0, 100000);
    EXPECT_LE(c.GetPitch(), 1.5533f + 1e-4f);
}

TEST(FlyCameraTest, MoveForwardChangesZ) {
    xe::FlyCameraController c;
    c.SetYaw(0.0f);
    c.SetPitch(0.0f);
    float x0, y0, z0;
    c.GetPosition(x0, y0, z0);
    c.Update(1.0f, true, false, false, false, false, false, 0, 0);
    float x1, y1, z1;
    c.GetPosition(x1, y1, z1);
    EXPECT_LT(z1, z0);  // moved toward -Z
}

TEST(FlyCameraTest, StrafeRightChangesX) {
    xe::FlyCameraController c;
    c.SetYaw(0.0f);
    c.SetPitch(0.0f);
    float x0, y0, z0;
    c.GetPosition(x0, y0, z0);
    c.Update(1.0f, false, false, false, true, false, false, 0, 0);
    float x1, y1, z1;
    c.GetPosition(x1, y1, z1);
    EXPECT_GT(x1, x0);
}

TEST(FlyCameraTest, YawRotatesFromMouseDelta) {
    xe::FlyCameraController c;
    const float y0 = c.GetYaw();
    c.Update(0.016f, false, false, false, false, false, false, -100, 0);
    EXPECT_GT(c.GetYaw(), y0);  // negative dx increases yaw
}

TEST(FlyCameraTest, DisabledIgnoresInput) {
    xe::FlyCameraController c;
    c.SetEnabled(false);
    float x0, y0, z0;
    c.GetPosition(x0, y0, z0);
    c.Update(1.0f, true, false, false, false, false, false, -100, 100);
    float x1, y1, z1;
    c.GetPosition(x1, y1, z1);
    EXPECT_FLOAT_EQ(x1, x0);
    EXPECT_FLOAT_EQ(y1, y0);
    EXPECT_FLOAT_EQ(z1, z0);
}