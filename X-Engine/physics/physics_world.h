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
    bool enabled_    = true;
    float restitution_ = 0.5f;
};

}  // namespace xe
