#include <gtest/gtest.h>
#include "core/camera.h"

TEST(CameraTest, DefaultViewIsNonIdentity) {
    xe::Camera cam;
    cam.SetAspect(16.0f / 9.0f);
    auto v = cam.GetView();
    // Camera is at -3 on Z, looking at origin — view is non-trivial.
    EXPECT_NE(v.m[2][3], 0.0f);
}

TEST(CameraTest, PerspectiveProducesValidMatrix) {
    xe::Camera cam;
    cam.SetPerspective(1.0472f, 16.0f / 9.0f, 0.1f, 100.0f);
    auto p = cam.GetProjection();
    EXPECT_FLOAT_EQ(p.m[0][0], (1.0f / std::tan(0.5236f)) / (16.0f / 9.0f));
    EXPECT_GT(p.m[3][2], -2.0f);  // sanity, not absurd
    EXPECT_LT(p.m[3][2], 0.0f);
}

TEST(CameraTest, SettersTriggerDirtyRebuild) {
    xe::Camera cam;
    cam.SetPosition(0.0f, 0.0f, -3.0f);
    cam.SetTarget(0.0f, 0.0f, 0.0f);
    cam.SetUp(0.0f, 1.0f, 0.0f);
    cam.SetPerspective(1.0f, 1.0f, 0.1f, 10.0f);
    auto v = cam.GetView();
    auto p = cam.GetProjection();
    // Translate -3 on Z, looking at origin → expect view m[2][3] = -3.0f
    EXPECT_NEAR(v.m[2][3], -3.0f, 1e-3f);
}

TEST(CameraTest, ViewProjectionIsComposable) {
    xe::Camera cam;
    cam.SetPosition(0.0f, 0.0f, -5.0f);
    cam.SetTarget(0.0f, 0.0f, 0.0f);
    cam.SetAspect(1.0f);
    cam.SetPerspective(1.5707f, 1.0f, 0.1f, 100.0f);
    auto vp = cam.GetViewProjection();
    // View Projection should not be identity
    EXPECT_NE(vp.m[0][0], 1.0f);
    EXPECT_NE(vp.m[0][0], 0.0f);
}