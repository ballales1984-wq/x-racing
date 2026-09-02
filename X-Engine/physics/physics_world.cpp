#include "physics/physics_world.h"
#include <cmath>

namespace xe {

int PhysicsWorld::Add(const RigidBody& body) {
    bodies_.push_back(body);
    return static_cast<int>(bodies_.size()) - 1;
}

void PhysicsWorld::Clear() {
    bodies_.clear();
    collisions_.clear();
}

void PhysicsWorld::Kick(int idx, Vec3 impulse) {
    if (idx < 0 || idx >= static_cast<int>(bodies_.size())) return;
    if (!bodies_[idx].dynamic) return;
    bodies_[idx].velocity.x += impulse.x / bodies_[idx].mass;
    bodies_[idx].velocity.y += impulse.y / bodies_[idx].mass;
    bodies_[idx].velocity.z += impulse.z / bodies_[idx].mass;
    bodies_[idx].awake = true;
}

void PhysicsWorld::Kick(int idx, float x, float y, float z) {
    Kick(idx, Vec3{ x, y, z });
}

int PhysicsWorld::Step(float dt, float damping, float restitution) {
    if (!enabled_) return 0;
    collisions_.clear();

    // 1. Integrate (semi-implicit Euler) with damping.
    for (auto& b : bodies_) {
        if (!b.dynamic) continue;
        b.position.x += b.velocity.x * dt;
        b.position.y += b.velocity.y * dt;
        b.position.z += b.velocity.z * dt;
        b.velocity.x *= damping;
        b.velocity.y *= damping;
        b.velocity.z *= damping;
    }

    // 2. Broadphase-free n^2 sphere-sphere detection + impulse.
    int resolved = 0;
    const int n = static_cast<int>(bodies_.size());
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            auto& A = bodies_[i];
            auto& B = bodies_[j];
            if (!A.dynamic && !B.dynamic) continue;
            float dx = B.position.x - A.position.x;
            float dy = B.position.y - A.position.y;
            float dz = B.position.z - A.position.z;
            float d2 = dx*dx + dy*dy + dz*dz;
            float r  = A.radius + B.radius;
            if (d2 >= r * r) continue;

            float d = std::sqrt(d2);
            CollisionInfo info;
            info.a = i; info.b = j;
            if (d > 1e-6f) {
                info.normal = { dx / d, dy / d, dz / d };
            }
            info.penetration = r - d;

            // Positional correction (split by mass).
            float invMa = A.dynamic ? 1.0f / A.mass : 0.0f;
            float invMb = B.dynamic ? 1.0f / B.mass : 0.0f;
            float invSum = invMa + invMb;
            if (invSum > 0.0f) {
                float corr = info.penetration / invSum;
                A.position.x -= info.normal.x * corr * invMa;
                A.position.y -= info.normal.y * corr * invMa;
                A.position.z -= info.normal.z * corr * invMa;
                B.position.x += info.normal.x * corr * invMb;
                B.position.y += info.normal.y * corr * invMb;
                B.position.z += info.normal.z * corr * invMb;
            }

            // Impulse along normal.
            float rvx = B.velocity.x - A.velocity.x;
            float rvy = B.velocity.y - A.velocity.y;
            float rvz = B.velocity.z - A.velocity.z;
            float velAlongNormal = rvx * info.normal.x + rvy * info.normal.y + rvz * info.normal.z;
            if (velAlongNormal > 0.0f) {
                // separating already
            } else if (invSum > 0.0f) {
                float j_imp = -(1.0f + restitution) * velAlongNormal / invSum;
                float jx = j_imp * info.normal.x;
                float jy = j_imp * info.normal.y;
                float jz = j_imp * info.normal.z;
                if (A.dynamic) {
                    A.velocity.x -= jx * invMa;
                    A.velocity.y -= jy * invMa;
                    A.velocity.z -= jz * invMa;
                }
                if (B.dynamic) {
                    B.velocity.x += jx * invMb;
                    B.velocity.y += jy * invMb;
                    B.velocity.z += jz * invMb;
                }
            }

            collisions_.push_back(info);
            resolved++;
        }
    }
    return resolved;
}

}  // namespace xe