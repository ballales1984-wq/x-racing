#pragma once

#include <cmath>
#include <array>

namespace xe {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct Mat4 {
    std::array<std::array<float, 4>, 4> m{};

    constexpr Mat4() = default;

    static Mat4 Identity() {
        Mat4 r;
        for (int i = 0; i < 4; ++i) r.m[i][i] = 1.0f;
        return r;
    }

    static Mat4 Zero() {
        return Mat4{};
    }

    static Mat4 RotationZ(float radians) {
        Mat4 r = Identity();
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        r.m[0][0] =  c; r.m[0][1] = -s;
        r.m[1][0] =  s; r.m[1][1] =  c;
        return r;
    }

    static Mat4 RotationX(float radians) {
        Mat4 r = Identity();
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        r.m[1][1] =  c; r.m[1][2] = -s;
        r.m[2][1] =  s; r.m[2][2] =  c;
        return r;
    }

    static Mat4 RotationY(float radians) {
        Mat4 r = Identity();
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        r.m[0][0] =  c; r.m[0][2] =  s;
        r.m[2][0] = -s; r.m[2][2] =  c;
        return r;
    }

    static Mat4 Translation(float x, float y, float z) {
        Mat4 r = Identity();
        r.m[0][3] = x;
        r.m[1][3] = y;
        r.m[2][3] = z;
        return r;
    }

    static Mat4 Scale(float sx, float sy, float sz) {
        Mat4 r = Identity();
        r.m[0][0] = sx;
        r.m[1][1] = sy;
        r.m[2][2] = sz;
        return r;
    }

    static Mat4 Orthographic(float left, float right, float bottom, float top,
                             float near_z, float far_z) {
        Mat4 r;
        r.m[0][0] =  2.0f / (right - left);
        r.m[1][1] =  2.0f / (top - bottom);
        r.m[2][2] =  1.0f / (near_z - far_z);
        r.m[0][3] = -(right + left) / (right - left);
        r.m[1][3] = -(top + bottom) / (top - bottom);
        r.m[2][3] =  near_z / (near_z - far_z);
        r.m[3][3] =  1.0f;
        return r;
    }

    static Mat4 Perspective(float fov_y_radians, float aspect, float near_z, float far_z) {
        const float f = 1.0f / std::tan(fov_y_radians * 0.5f);
        Mat4 r = Zero();
        r.m[0][0] = f / aspect;
        r.m[1][1] = f;
        r.m[2][2] = far_z / (near_z - far_z);
        r.m[2][3] = (far_z * near_z) / (near_z - far_z);
        r.m[3][2] = -1.0f;
        return r;
    }

    static Mat4 LookAt(float ex, float ey, float ez,
                       float lx, float ly, float lz,
                       float ux, float uy, float uz) {
        // forward = normalize(eye - target)
        float fxv = ex - lx, fyv = ey - ly, fzv = ez - lz;
        float fl = std::sqrt(fxv*fxv + fyv*fyv + fzv*fzv);
        if (fl < 1e-6f) return Identity();
        fxv /= fl; fyv /= fl; fzv /= fl;

        // side = normalize(forward × up)
        float sidex = fyv*uz - fzv*uy;
        float sidey = fzv*ux - fxv*uz;
        float sidez = fxv*uy - fyv*ux;
        float sl = std::sqrt(sidex*sidex + sidey*sidey + sidez*sidez);
        if (sl < 1e-6f) return Identity();
        sidex /= sl; sidey /= sl; sidez /= sl;

        // up' = side × forward
        float upx = sidey*fzv - sidez*fyv;
        float upy = sidez*fxv - sidex*fzv;
        float upz = sidex*fyv - sidey*fxv;

        Mat4 r = Identity();
        r.m[0][0] =  sidex; r.m[0][1] =  sidey; r.m[0][2] =  sidez;
        r.m[1][0] =  upx;   r.m[1][1] =  upy;   r.m[1][2] =  upz;
        r.m[2][0] =  fxv;   r.m[2][1] =  fyv;   r.m[2][2] =  fzv;

        r.m[0][3] = -(sidex*ex + sidey*ey + sidez*ez);
        r.m[1][3] = -(upx*ex   + upy*ey   + upz*ez);
        r.m[2][3] = -(fxv*ex   + fyv*ey   + fzv*ez);
        return r;
    }

    static Mat4 Multiply(const Mat4& a, const Mat4& b) {
        Mat4 r;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += a.m[k][j] * b.m[i][k];
                }
                r.m[i][j] = sum;
            }
        }
        return r;
    }
};

}  // namespace xe