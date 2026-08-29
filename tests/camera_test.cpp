#include <gtest/gtest.h>
#include "camera/camera.h"

using namespace p0;
using namespace p0::camera;

// Configurable distance/height: with dt=0 the camera snaps to the desired pose.
TEST(ChaseCamera, ConfigurableDistanceAndHeight) {
  CameraConfig cfg;
  cfg.distance = 12.0;
  cfg.height = 5.0;
  cfg.look_ahead = 3.0;
  cfg.smoothing = 8.0;

  ChaseCamera cam(cfg);
  // Car at origin, heading 0 (forward = +X).
  cam.update(Vec2(0.0, 0.0), 0.0, 0.0, 0.0);

  // Eye is behind (-X) and above (+Y) the car.
  EXPECT_NEAR(cam.state().position.x(), -cfg.distance, 1e-6);
  EXPECT_NEAR(cam.state().position.y(), cfg.height, 1e-6);
  EXPECT_NEAR(cam.state().position.z(), 0.0, 1e-6);

  // Target sits ahead of the car along +X.
  EXPECT_NEAR(cam.state().target.x(), cfg.look_ahead, 1e-6);
  EXPECT_NEAR(cam.state().target.y(), 0.0, 1e-6);
  EXPECT_NEAR(cam.state().target.z(), 0.0, 1e-6);
}

// Smoothing: after a large pose change the camera moves only part-way.
TEST(ChaseCamera, SmoothingInterpolates) {
  CameraConfig cfg;
  cfg.distance = 8.0;
  cfg.height = 4.0;
  cfg.smoothing = 2.0;  // gentle

  ChaseCamera cam(cfg);
  cam.update(Vec2(0.0, 0.0), 0.0, 0.0, 0.0);  // snap to origin pose
  const Vec3 first = cam.state().position;

  // Move the car far away and step one frame.
  cam.update(Vec2(100.0, 0.0), 0.0, 0.0, 1.0 / 60.0);
  const Vec3 moved = cam.state().position;

  // Position should have moved toward the new desired point but not reached it.
  const double dist_moved = (moved - first).norm();
  EXPECT_GT(dist_moved, 0.0);
  EXPECT_LT(dist_moved, 8.0);  // far from the full 100-unit shift
}

// A higher smoothing rate moves closer to the target in the same time step.
TEST(ChaseCamera, HigherSmoothingMovesFaster) {
  auto step = [](double smoothing) {
    CameraConfig cfg;
    cfg.distance = 8.0;
    cfg.smoothing = smoothing;
    ChaseCamera cam(cfg);
    cam.update(Vec2(0.0, 0.0), 0.0, 0.0, 0.0);
    cam.update(Vec2(50.0, 0.0), 0.0, 0.0, 1.0 / 60.0);
    return (cam.state().position - Vec3(-8.0, 4.0, 0.0)).norm();
  };
  const double slow = step(1.0);
  const double fast = step(20.0);
  EXPECT_GT(fast, slow);
}

// The view matrix built from the camera pose is a proper orthonormal basis.
TEST(ChaseCamera, ViewMatrixIsOrthonormal) {
  CameraConfig cfg;
  cfg.distance = 8.0;
  cfg.height = 4.0;
  ChaseCamera cam(cfg);
  cam.update(Vec2(3.0, 5.0), 0.7, 0.0, 0.0);

  Mat4 v = cam.view_matrix();
  // Upper-left 3x3 should be orthonormal (R^T R = I).
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      double dot = 0.0;
      for (int k = 0; k < 3; ++k) dot += v(i, k) * v(j, k);
      EXPECT_NEAR(dot, (i == j ? 1.0 : 0.0), 1e-9);
    }
  }
}

// The car (at the target) should project to a valid on-screen pixel.
TEST(ChaseCamera, ProjectsCarToScreen) {
  CameraConfig cfg;
  cfg.distance = 8.0;
  cfg.height = 4.0;
  cfg.look_ahead = 2.0;
  ChaseCamera cam(cfg);
  cam.update(Vec2(0.0, 0.0), 0.0, 0.0, 0.0);

  Vec3 car(0.0, 0.0, 0.0);
  Vec3 screen = cam.project(car, 1280, 720);
  EXPECT_GE(screen.x(), 0.0);
  EXPECT_LE(screen.x(), 1280.0);
  EXPECT_GE(screen.y(), 0.0);
  EXPECT_LE(screen.y(), 720.0);
}
