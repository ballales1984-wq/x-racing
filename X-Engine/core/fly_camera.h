#pragma once

#include "core/math.h"
#include "core/camera.h"

namespace xe {

// FPS-style free-fly camera controller.
// WASD = strafe/forward/back; Space = up; Ctrl = down.
// Mouse delta = yaw/pitch (radians). Q/E = roll.
class FlyCameraController {
public:
    FlyCameraController() = default;

    void SetMoveSpeed(float units_per_sec) { move_speed_ = units_per_sec; }
    void SetLookSpeed(float radians_per_pixel) { look_speed_ = radians_per_pixel; }
    void SetSensitivity(float sens) { sensitivity_ = sens; }

    void SetYaw(float y) { yaw_ = y; }
    void SetPitch(float p) { pitch_ = p; }
    float GetYaw() const { return yaw_; }
    float GetPitch() const { return pitch_; }

    void SetEnabled(bool e) { enabled_ = e; }
    bool IsEnabled() const { return enabled_; }

    void SetPosition(float x, float y, float z) {
        pos_.x = x; pos_.y = y; pos_.z = z;
    }
    void GetPosition(float& x, float& y, float& z) const {
        x = pos_.x; y = pos_.y; z = pos_.z;
    }

    Vec3 GetForward() const {
        const float cp = std::cos(pitch_);
        return Vec3{ cp * std::sin(yaw_), std::sin(pitch_), -cp * std::cos(yaw_) };
    }

    Vec3 GetRight() const {
        // World up × forward (right-handed), with pitch-free right for fps-style.
        Vec3 fwd = GetForward();
        Vec3 world_up{ 0.0f, 1.0f, 0.0f };
        // right = fwd × up, normalized
        Vec3 r{
            fwd.y * world_up.z - fwd.z * world_up.y,
            fwd.z * world_up.x - fwd.x * world_up.z,
            fwd.x * world_up.y - fwd.y * world_up.x,
        };
        float rl = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
        if (rl > 1e-6f) { r.x /= rl; r.y /= rl; r.z /= rl; }
        return r;
    }

    // input: fwd, back, left, right, up, down ∈ {0,1}
    // mouse_dx, mouse_dy in pixels
    void Update(float dt, bool fwd, bool back, bool left, bool right,
                bool up_key, bool down_key, int mouse_dx, int mouse_dy) {
        if (!enabled_) return;

        yaw_   -= static_cast<float>(mouse_dx) * look_speed_ * sensitivity_;
        pitch_ -= static_cast<float>(mouse_dy) * look_speed_ * sensitivity_;
        constexpr float kPitchLimit = 1.5533f;  // ~89 deg
        if (pitch_ >  kPitchLimit) pitch_ =  kPitchLimit;
        if (pitch_ < -kPitchLimit) pitch_ = -kPitchLimit;

        Vec3 fwd_v = GetForward();
        Vec3 right_v = GetRight();

        float step = move_speed_ * dt;
        if (fwd) {
            pos_.x += fwd_v.x * step;
            pos_.y += fwd_v.y * step;
            pos_.z += fwd_v.z * step;
        }
        if (back) {
            pos_.x -= fwd_v.x * step;
            pos_.y -= fwd_v.y * step;
            pos_.z -= fwd_v.z * step;
        }
        if (right) {
            pos_.x += right_v.x * step;
            pos_.y += right_v.y * step;
            pos_.z += right_v.z * step;
        }
        if (left) {
            pos_.x -= right_v.x * step;
            pos_.y -= right_v.y * step;
            pos_.z -= right_v.z * step;
        }
        if (up_key)    pos_.y += step;
        if (down_key)  pos_.y -= step;
    }

    // Apply current state to a Camera object (so it builds the right view matrix).
    void ApplyTo(Camera& cam) const {
        cam.SetPosition(pos_.x, pos_.y, pos_.z);
        Vec3 fwd = GetForward();
        cam.SetTarget(pos_.x + fwd.x, pos_.y + fwd.y, pos_.z + fwd.z);
        cam.SetUp(0.0f, 1.0f, 0.0f);
    }

private:
    Vec3 pos_{ 0.0f, 1.5f, -4.0f };
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float move_speed_ = 4.0f;
    float look_speed_ = 0.0025f;
    float sensitivity_ = 1.0f;
    bool enabled_ = true;
};

}  // namespace xe