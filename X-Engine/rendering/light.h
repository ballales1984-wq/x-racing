#pragma once

#include "core/math.h"
#include <array>
#include <cmath>

namespace xe {

struct DirectionalLight {
    Vec3 direction{ -0.3f, 1.0f, -0.5f };   // unit vector pointing FROM surface TOWARD light
    std::array<float, 3> color{ 1.0f, 0.97f, 0.9f };
    float intensity = 1.2f;

    std::array<float, 3> Ambient() const {
        return { 0.10f, 0.10f, 0.12f };
    }
};

// CPU-side Lambert + Blinn-Phong for testing and HUD overlay.
struct LightingResult {
    std::array<float, 3> diffuse;
    std::array<float, 3> specular;
};

inline LightingResult ComputeLighting(const Vec3& normal,
                                      const Vec3& world_pos,
                                      const DirectionalLight& light,
                                      const Vec3& camera_pos,
                                      float shininess = 32.0f,
                                      float specular_strength = 0.5f) {
    // Normalize normal defensively
    float nl = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    Vec3 n = nl > 1e-6f ? Vec3{ normal.x / nl, normal.y / nl, normal.z / nl } : Vec3{ 0, 1, 0 };

    // Normalize light direction (points FROM surface TOWARD the light)
    float dl = std::sqrt(light.direction.x * light.direction.x +
                         light.direction.y * light.direction.y +
                         light.direction.z * light.direction.z);
    Vec3 l = dl > 1e-6f ? Vec3{ light.direction.x / dl, light.direction.y / dl, light.direction.z / dl }
                       : Vec3{ 0, 1, 0 };

    float ndotl = std::max(0.0f, n.x * l.x + n.y * l.y + n.z * l.z);

    LightingResult r{};
    for (int i = 0; i < 3; ++i) {
        r.diffuse[i] = light.intensity * ndotl * light.color[i];
    }

    // Blinn-Phong: view dir + half vector
    Vec3 v{ camera_pos.x - world_pos.x,
            camera_pos.y - world_pos.y,
            camera_pos.z - world_pos.z };
    float vl = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (vl > 1e-6f) { v.x /= vl; v.y /= vl; v.z /= vl; }
    Vec3 h{ l.x + v.x, l.y + v.y, l.z + v.z };
    float hl = std::sqrt(h.x * h.x + h.y * h.y + h.z * h.z);
    if (hl > 1e-6f) { h.x /= hl; h.y /= hl; h.z /= hl; }

    float ndoth = std::max(0.0f, n.x * h.x + n.y * h.y + n.z * h.z);
    float spec = std::pow(ndoth, shininess) * specular_strength;
    for (int i = 0; i < 3; ++i) {
        r.specular[i] = spec * light.color[i];
    }
    return r;
}

}  // namespace xe