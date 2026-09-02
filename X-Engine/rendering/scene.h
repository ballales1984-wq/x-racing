#pragma once

#include "core/math.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace xe {

struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
    float color[4];
};

enum class MeshKind : uint8_t {
    Triangle,
    Cube,
    Quad,
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    MeshKind kind = MeshKind::Cube;

    static MeshData MakeCube();
    static MeshData MakeTriangle();
    static MeshData MakeQuad();
    static MeshData MakeTexturedQuad();
};

struct MeshInstance {
    MeshKind mesh = MeshKind::Cube;
    Vec3 position{ 0.0f, 0.0f, 0.0f };
    Vec3 rotation_rad{ 0.0f, 0.0f, 0.0f };
    Vec3 scale{ 1.0f, 1.0f, 1.0f };
    std::array<float, 4> tint{ 1.0f, 1.0f, 1.0f, 1.0f };
    std::string texture_path;  // empty = no texture

    Mat4 GetWorldMatrix() const {
        Mat4 rx = Mat4::RotationX(rotation_rad.x);
        Mat4 ry = Mat4::RotationY(rotation_rad.y);
        Mat4 rz = Mat4::RotationZ(rotation_rad.z);
        Mat4 r  = Mat4::Multiply(rz, Mat4::Multiply(ry, rx));
        Mat4 s  = Mat4::Scale(scale.x, scale.y, scale.z);
        Mat4 t  = Mat4::Translation(position.x, position.y, position.z);
        return Mat4::Multiply(t, Mat4::Multiply(r, s));
    }
};

struct SceneObject {
    std::string name;
    MeshInstance instance;
};

struct Scene {
    std::vector<SceneObject> objects;
    Vec3 camera_position{ 0.0f, 1.5f, -4.0f };
    Vec3 camera_target{ 0.0f, 0.0f, 0.0f };
    Vec3 camera_up{ 0.0f, 1.0f, 0.0f };
    float camera_fov_y = 1.0472f;
    float camera_near_z = 0.1f;
    float camera_far_z = 100.0f;

    Mat4 GetViewMatrix() const {
        return Mat4::LookAt(camera_position.x, camera_position.y, camera_position.z,
                             camera_target.x,   camera_target.y,   camera_target.z,
                             camera_up.x,       camera_up.y,       camera_up.z);
    }

    Mat4 GetProjectionMatrix(float aspect) const {
        return Mat4::Perspective(camera_fov_y, aspect, camera_near_z, camera_far_z);
    }
};

}  // namespace xe