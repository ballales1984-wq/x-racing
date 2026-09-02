#pragma once

#include "core/math.h"
#include <vector>

namespace xe {

struct RigidBody {
    Vec3 position{ 0.0f, 0.0f, 0.0f };
    Vec3 velocity{ 0.0f, 0.0f, 0.0f };
    float radius = 0.5f;   // bounding sphere
    float mass = 1.0f;
    bool  dynamic = true;  // if false, treated as immovable
    bool  awake   = true;
};

struct CollisionInfo {
    int   a = -1;
    int   b = -1;
    Vec3  normal{ 0.0f, 1.0f, 0.0f };   // points from A toward B
    float penetration = 0.0f;
};

class PhysicsWorld {
public:
    PhysicsWorld() = default;

    // Add a body. Returns its index.
    int Add(const RigidBody& body);

    // Remove all bodies.
    void Clear();

    // Step the simulation by `dt`. Returns number of collisions resolved.
    int Step(float dt, float damping = 0.999f, float restitution = 0.6f);

    // Kick a body by index with a velocity delta.
    void Kick(int idx, Vec3 impulse);
    void Kick(int idx, float x, float y, float z);

    RigidBody&       Get(int idx)       { return bodies_[idx]; }
    const RigidBody& Get(int idx) const { return bodies_[idx]; }
    int Size() const { return static_cast<int>(bodies_.size()); }

    void SetEnabled(bool e) { enabled_ = e; }
    bool IsEnabled() const { return enabled_; }

    const std::vector<CollisionInfo>& LastCollisions() const { return collisions_; }
    int LastCollisionCount() const { return static_cast<int>(collisions_.size()); }

private:
    std::vector<RigidBody> bodies_;
    std::vector<CollisionInfo> collisions_;
    bool enabled_ = true;
};

}  // namespace xe