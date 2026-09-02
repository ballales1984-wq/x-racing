#pragma once

#include "core/math.h"

namespace xe {

class Camera {
public:
    Camera() = default;

    void SetPosition(float x, float y, float z) { position_[0] = x; position_[1] = y; position_[2] = z; dirty_ = true; }
    void SetTarget(float x, float y, float z)   { target_[0]   = x; target_[1]   = y; target_[2]   = z; dirty_ = true; }
    void SetUp(float x, float y, float z)       { up_[0]       = x; up_[1]       = y; up_[2]       = z; dirty_ = true; }

    void SetPerspective(float fov_y_radians, float aspect, float near_z, float far_z) {
        fov_y_ = fov_y_radians;
        aspect_ = aspect;
        near_z_ = near_z;
        far_z_ = far_z;
        dirty_ = true;
    }

    void SetAspect(float aspect) { aspect_ = aspect; dirty_ = true; }

    const Mat4& GetView() {
        if (dirty_) Rebuild();
        return view_;
    }

    const Mat4& GetProjection() {
        if (dirty_) Rebuild();
        return projection_;
    }

    Mat4 GetViewProjection() {
        if (dirty_) Rebuild();
        return Mat4::Multiply(projection_, view_);
    }

    void GetPosition(float& x, float& y, float& z) const {
        x = position_[0]; y = position_[1]; z = position_[2];
    }

private:
    void Rebuild() {
        view_ = Mat4::LookAt(position_[0], position_[1], position_[2],
                             target_[0],   target_[1],   target_[2],
                             up_[0],       up_[1],       up_[2]);
        projection_ = Mat4::Perspective(fov_y_, aspect_, near_z_, far_z_);
        dirty_ = false;
    }

    float position_[3] = { 0.0f, 0.0f, -3.0f };
    float target_[3]   = { 0.0f, 0.0f,  0.0f };
    float up_[3]       = { 0.0f, 1.0f,  0.0f };
    float fov_y_       = 1.0472f;  // ~60 deg
    float aspect_      = 16.0f / 9.0f;
    float near_z_      = 0.1f;
    float far_z_       = 100.0f;
    Mat4 view_;
    Mat4 projection_;
    bool dirty_ = true;
};

}  // namespace xe