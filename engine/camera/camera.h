// X-Racing — chase camera
// Author: alessio
#pragma once

#include "common.h"

// Project 0 — camera subsystem
// Namespace: p0::camera
namespace p0::camera {

// Tunable chase-camera parameters.
struct CameraConfig {
  double distance = 8.0;     // m, eye distance behind the car
  double height = 4.0;       // m, eye height above the car
  double look_ahead = 2.0;   // m, how far ahead of the car the target sits
  double smoothing = 8.0;    // 1/s, exponential smoothing rate (higher = snappier)
};

// Smoothed world-space pose of the chase camera.
struct CameraState {
  Vec3 position{0.0, 0.0, 0.0};  // eye position
  Vec3 target{0.0, 0.0, 0.0};    // look-at point
};

// ChaseCamera: a follow camera that sits behind/above the car, looks slightly
// ahead of it, and smoothly interpolates its pose to avoid jitter. It is a pure
// math class (no rendering dependency) so it can be unit-tested headlessly.
class ChaseCamera {
 public:
  explicit ChaseCamera(const CameraConfig& cfg = {}) : cfg_(cfg) {}

  void set_config(const CameraConfig& cfg) { cfg_ = cfg; }
  const CameraConfig& config() const { return cfg_; }

  // Update the smoothed camera pose from the vehicle pose.
  //   car_position : world XY of the car (z is treated as 0)
  //   heading      : yaw angle of the car (rad)
  //   dt           : frame time (s)
  void update(const Vec2& car_position, double heading, double /*speed*/, double dt) {
    const Vec3 car(car_position.x(), 0.0, car_position.y());

    // Desired eye: behind (-forward) and above the car.
    const Vec3 forward(std::cos(heading), 0.0, std::sin(heading));
    const Vec3 up(0.0, 1.0, 0.0);
    const Vec3 desired_pos = car - forward * cfg_.distance + up * cfg_.height;
    const Vec3 desired_target = car + forward * cfg_.look_ahead;

    const double alpha = (dt > 0.0)
        ? (1.0 - std::exp(-cfg_.smoothing * dt))
        : 1.0;

    if (!initialized_) {
      state_.position = desired_pos;
      state_.target = desired_target;
      initialized_ = true;
    } else {
      state_.position = lerp(state_.position, desired_pos, alpha);
      state_.target = lerp(state_.target, desired_target, alpha);
    }
  }

  // Build an orthonormal look-at view matrix from the current pose.
  Mat4 view_matrix() const {
    const Vec3 f = (state_.target - state_.position).normalized();
    const Vec3 up(0.0, 1.0, 0.0);
    const Vec3 s = f.cross(up).normalized();
    const Vec3 u = s.cross(f);

    Mat4 view = Mat4::Identity();
    view(0, 0) = s.x(); view(0, 1) = s.y(); view(0, 2) = s.z(); view(0, 3) = -s.dot(state_.position);
    view(1, 0) = u.x(); view(1, 1) = u.y(); view(1, 2) = u.z(); view(1, 3) = -u.dot(state_.position);
    view(2, 0) = -f.x(); view(2, 1) = -f.y(); view(2, 2) = -f.z(); view(2, 3) = f.dot(state_.position);
    return view;
  }

  // Project a world point to screen pixels using a perspective projection.
  // Returns (-9999, -9999, 0) for points behind the camera.
  Vec3 project(const Vec3& world_pos, int width, int height) const {
    Mat4 view = view_matrix();
    Vec4 view_pos = view * Vec4(world_pos.x(), world_pos.y(), world_pos.z(), 1.0);
    if (view_pos.z() >= -0.1) return Vec3(-9999.0, -9999.0, 0.0);

    const double fov = 60.0 * kDegToRad;
    const double aspect = static_cast<double>(width) / static_cast<double>(height);
    const double near_plane = 0.1;
    const double far_plane = 200.0;
    const double f = 1.0 / std::tan(fov / 2.0);

    Mat4 proj = Mat4::Identity();
    proj(0, 0) = f / aspect;
    proj(1, 1) = f;
    proj(2, 2) = (far_plane + near_plane) / (near_plane - far_plane);
    proj(2, 3) = (2.0 * far_plane * near_plane) / (near_plane - far_plane);
    proj(3, 2) = -1.0;
    proj(3, 3) = 0.0;

    Vec4 clip = proj * view_pos;
    if (clip.w() <= 0.0) return Vec3(-9999.0, -9999.0, 0.0);

    Vec3 ndc(clip.x() / clip.w(), clip.y() / clip.w(), clip.z() / clip.w());
    const int sx = static_cast<int>((ndc.x() * 0.5 + 0.5) * width);
    const int sy = static_cast<int>((1.0 - (ndc.y() * 0.5 + 0.5)) * height);
    return Vec3(static_cast<double>(sx), static_cast<double>(sy), ndc.z());
  }

  const CameraState& state() const { return state_; }

 private:
  CameraConfig cfg_;
  CameraState state_;
  bool initialized_ = false;
};

}  // namespace p0::camera
