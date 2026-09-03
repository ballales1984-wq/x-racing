#pragma once

#include "core/math.h"
#include <vector>

namespace xe {

// Shape kind for collision detection.
enum class ShapeKind {
    Sphere,
    Box,    // OBB (oriented bounding box) defined by half-extents + orientation
};

struct RigidBody {
    Vec3  position{ 0.0f, 0.0f, 0.0f };
    Vec3  velocity{ 0.0f, 0.0f, 0.0f };
    Vec3  angVel{ 0.0f, 0.0f, 0.0f };   // angular velocity (axis * radians/sec)
    Quat  orientation = Quat::Identity();

    ShapeKind shape = ShapeKind::Sphere;
    float radius    = 0.5f;              // sphere radius
    Vec3  halfExtents{ 0.5f, 0.5f, 0.5f }; // box half-extents (when shape == Box)

    float mass     = 1.0f;
    bool  dynamic  = true;
    bool  awake    = true;
    int   tag      = 0;                  // user-defined id (e.g. scene-object index)
};

struct CollisionInfo {
    int   a = -1;
    int   b = -1;
    Vec3  normal{ 0.0f, 1.0f, 0.0f };   // points from A toward B
    float penetration = 0.0f;
    Vec3  contact{ 0.0f, 0.0f, 0.0f };  // approximate contact point
};

// Axis-aligned bounding box (world space). Used for broadphase.
struct AABB {
    Vec3  min{ 0.0f, 0.0f, 0.0f };
    Vec3  max{ 0.0f, 0.0f, 0.0f };

    bool Overlaps(const AABB& o) const {
        return (min.x <= o.max.x && max.x >= o.min.x) &&
               (min.y <= o.max.y && max.y >= o.min.y) &&
               (min.z <= o.max.z && max.z >= o.min.z);
    }

    static AABB FromSphere(const Vec3& c, float r) {
        AABB a;
        a.min = { c.x - r, c.y - r, c.z - r };
        a.max = { c.x + r, c.y + r, c.z + r };
        return a;
    }

    // Conservative AABB from OBB (axis-aligned bounding box of a rotated box).
    static AABB FromOBB(const Vec3& center, const Vec3 axes[3], const Vec3& half) {
        // Project all 8 corners, take min/max.
        float minX = center.x, maxX = center.x;
        float minY = center.y, maxY = center.y;
        float minZ = center.z, maxZ = center.z;
        for (int sx = -1; sx <= 1; sx += 2)
        for (int sy = -1; sy <= 1; sy += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            Vec3 c{
                center.x + (axes[0].x * half.x * (float)sx) + (axes[1].x * half.y * (float)sy) + (axes[2].x * half.z * (float)sz),
                center.y + (axes[0].y * half.x * (float)sx) + (axes[1].y * half.y * (float)sy) + (axes[2].y * half.z * (float)sz),
                center.z + (axes[0].z * half.x * (float)sx) + (axes[1].z * half.y * (float)sy) + (axes[2].z * half.z * (float)sz)
            };
            if (c.x < minX) minX = c.x; if (c.x > maxX) maxX = c.x;
            if (c.y < minY) minY = c.y; if (c.y > maxY) maxY = c.y;
            if (c.z < minZ) minZ = c.z; if (c.z > maxZ) maxZ = c.z;
        }
        AABB a;
        a.min = { minX, minY, minZ };
        a.max = { maxX, maxY, maxZ };
        return a;
    }
};

// Ray for picking.
struct Ray {
    Vec3 origin{ 0.0f, 0.0f, 0.0f };
    Vec3 dir{ 0.0f, 0.0f, -1.0f };   // unit length
};

// Result of a ray cast (closest hit).
struct RayHit {
    int   body  = -1;     // index into PhysicsWorld
    float t     = 0.0f;   // distance along ray
    Vec3  point{ 0.0f, 0.0f, 0.0f };
};

class PhysicsWorld {
public:
    PhysicsWorld() = default;

    int  Add(const RigidBody& body);
    void Clear();

    // Step the simulation. Returns number of collisions resolved.
    int Step(float dt, float damping = 0.999f, float restitution = 0.5f);

    // Apply linear impulse.
    void Kick(int idx, Vec3 impulse);
    void Kick(int idx, float x, float y, float z);

    // Apply angular impulse (tweak angVel directly — for the "spin" demo).
    void Spin(int idx, Vec3 axisRadiansPerSec);

    // Reset all body positions/velocities to their initial state.
    void ResetAll();

    // Store the "initial" state snapshot. Call before simulation begins.
    void CaptureInitialState();

    // Broadphase: compute AABB pair list (overlapping pairs).
    // Returned vector is sorted by (a,b) for determinism.
    std::vector<std::pair<int,int>> ComputeBroadphasePairs() const;

    // World-space AABB of body i.
    AABB GetAABB(int i) const;

    // Cast a ray and return the closest hit, or RayHit{ -1, 0, {} } on miss.
    // maxDist caps the search (e.g. 100m).
    RayHit RayCast(const Ray& ray, float maxDist = 1000.0f) const;

    // Selected body (for highlight). -1 = none.
    void SetSelected(int idx);
    int  Selected() const { return selected_; }

    RigidBody&       Get(int idx)       { return bodies_[idx]; }
    const RigidBody& Get(int idx) const { return bodies_[idx]; }
    int Size() const { return static_cast<int>(bodies_.size()); }

    void SetEnabled(bool e) { enabled_ = e; }
    bool IsEnabled() const { return enabled_; }

    const std::vector<CollisionInfo>& LastCollisions() const { return collisions_; }
    int LastCollisionCount() const { return static_cast<int>(collisions_.size()); }

    // Configuration.
    void SetRestitution(float r) { restitution_ = r; }
    float Restitution() const    { return restitution_; }

private:
    std::vector<RigidBody>        bodies_;
    std::vector<RigidBody>        initial_;
    std::vector<CollisionInfo>    collisions_;
    std::vector<AABB>             aabbs_;       // parallel to bodies_
    bool enabled_    = true;
    float restitution_ = 0.5f;
    int  selected_  = -1;
};

}  // namespace xe
