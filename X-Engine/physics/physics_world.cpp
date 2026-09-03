#include "physics/physics_world.h"
#include <cmath>
#include <algorithm>

namespace xe {

namespace {

inline Quat QuatFromAngVel(const Vec3& w, float dt) {
    float mag = Length(w);
    if (mag < 1e-6f) return Quat::Identity();
    return Quat::FromAxisAngle({ w.x/mag, w.y/mag, w.z/mag }, mag * dt);
}

inline void IntegrateOrientation(Quat& q, const Vec3& w, float dt) {
    Quat dq = QuatFromAngVel(w, dt);
    q = dq * q;
    q.Normalize();
}

// --- Collision math ---------------------------------------------------------

struct OBB {
    Vec3 center;
    Vec3 axes[3];       // local x,y,z axes in world space (unit length)
    Vec3 half;          // half-extents
};

// Build OBB from a rigid body. Identity quaternion -> world axes.
inline OBB MakeOBB(const RigidBody& b) {
    OBB o;
    o.center = b.position;
    o.axes[0] = b.orientation.Rotate({ 1.0f, 0.0f, 0.0f });
    o.axes[1] = b.orientation.Rotate({ 0.0f, 1.0f, 0.0f });
    o.axes[2] = b.orientation.Rotate({ 0.0f, 0.0f, 1.0f });
    o.half    = b.halfExtents;
    return o;
}

// Get the 8 corners of an OBB in world space.
inline void OBB_Corners(const OBB& o, Vec3 out[8]) {
    Vec3 e[3] = { o.axes[0]*o.half.x, o.axes[1]*o.half.y, o.axes[2]*o.half.z };
    int idx = 0;
    for (int sx = -1; sx <= 1; sx += 2)
    for (int sy = -1; sy <= 1; sy += 2)
    for (int sz = -1; sz <= 1; sz += 2) {
        out[idx++] = o.center + e[0]*(float)sx + e[1]*(float)sy + e[2]*(float)sz;
    }
}

// Project OBB onto an axis. Returns (min, max).
inline void OBB_Project(const OBB& o, const Vec3& axis, float& mn, float& mx) {
    float r = o.half.x * std::fabs(Dot(o.axes[0], axis))
            + o.half.y * std::fabs(Dot(o.axes[1], axis))
            + o.half.z * std::fabs(Dot(o.axes[2], axis));
    float c = Dot(o.center, axis);
    mn = c - r; mx = c + r;
}

// Separating Axis Theorem test for two OBBs. Returns penetration depth
// and the axis (pointing from A to B) on hit, or (0,+) on miss.
struct SatHit { bool hit; Vec3 axis; float depth; Vec3 contact; };
SatHit OBB_vs_OBB(const OBB& A, const OBB& B) {
    SatHit r{ false, {0,1,0}, 0.0f, (A.center+B.center)*0.5f };
    float minDepth = 1e30f;
    Vec3  bestAxis = { 0, 1, 0 };

    // 3 axes from A, 3 from B, 9 from cross products.
    Vec3 axes[15];
    int nAxis = 0;
    for (int i = 0; i < 3; ++i) axes[nAxis++] = A.axes[i];
    for (int i = 0; i < 3; ++i) axes[nAxis++] = B.axes[i];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            Vec3 cr = Cross(A.axes[i], B.axes[j]);
            float l = Length(cr);
            if (l < 1e-6f) continue;
            axes[nAxis++] = cr * (1.0f / l);
        }
    }

    for (int i = 0; i < nAxis; ++i) {
        const Vec3& ax = axes[i];
        float aMin, aMax, bMin, bMax;
        OBB_Project(A, ax, aMin, aMax);
        OBB_Project(B, ax, bMin, bMax);
        if (aMax < bMin || bMax < aMin) {
            r.hit = false; return r;
        }
        float overlap = std::min(aMax, bMax) - std::max(aMin, bMin);
        if (overlap < minDepth) {
            minDepth = overlap;
            bestAxis = ax;
        }
    }
    // Ensure axis points from A to B.
    Vec3 dir = B.center - A.center;
    if (Dot(bestAxis, dir) < 0.0f) bestAxis = bestAxis * -1.0f;

    r.hit   = true;
    r.axis  = bestAxis;
    r.depth = minDepth;

    // Approximate contact: midpoint of OBBs' centers.
    r.contact = (A.center + B.center) * 0.5f;
    return r;
}

// OBB vs sphere. Returns hit + closest point on OBB to sphere center.
SatHit OBB_vs_Sphere(const OBB& o, const Vec3& c, float radius) {
    SatHit r{ false, {0,1,0}, 0.0f, c };
    // Transform sphere center into OBB local space.
    Vec3 d = c - o.center;
    // Local axis components.
    float lx = Dot(d, o.axes[0]);
    float ly = Dot(d, o.axes[1]);
    float lz = Dot(d, o.axes[2]);
    // Closest point on box (clamp).
    float qx = std::clamp(lx, -o.half.x, o.half.x);
    float qy = std::clamp(ly, -o.half.y, o.half.y);
    float qz = std::clamp(lz, -o.half.z, o.half.z);
    // Difference in local space.
    float dx = lx - qx, dy = ly - qy, dz = lz - qz;
    float dist2 = dx*dx + dy*dy + dz*dz;
    if (dist2 > radius * radius) { r.hit = false; return r; }

    // If sphere center is INSIDE the box, use a safe axis (closest face).
    Vec3 normalLocal;
    float dist;
    if (dist2 < 1e-6f) {
        // Pick the axis with smallest penetration.
        float px = o.half.x - std::fabs(lx);
        float py = o.half.y - std::fabs(ly);
        float pz = o.half.z - std::fabs(lz);
        if (px < py && px < pz) {
            normalLocal = { (lx >= 0 ? 1.0f : -1.0f), 0, 0 };
            dist = -px;
        } else if (py < pz) {
            normalLocal = { 0, (ly >= 0 ? 1.0f : -1.0f), 0 };
            dist = -py;
        } else {
            normalLocal = { 0, 0, (lz >= 0 ? 1.0f : -1.0f) };
            dist = -pz;
        }
        // Contact point is sphere center.
        r.contact = c;
    } else {
        float dist01 = std::sqrt(dist2);
        normalLocal = { dx / dist01, dy / dist01, dz / dist01 };
        dist = dist01;
        // Contact point in local space.
        Vec3 qLocal = { qx, qy, qz };
        r.contact = o.center + o.axes[0]*qLocal.x + o.axes[1]*qLocal.y + o.axes[2]*qLocal.z;
    }

    // Convert local normal to world space.
    Vec3 nWorld = o.axes[0]*normalLocal.x + o.axes[1]*normalLocal.y + o.axes[2]*normalLocal.z;
    float nl = Length(nWorld);
    if (nl > 1e-6f) nWorld = nWorld * (1.0f / nl);

    r.hit   = true;
    r.axis  = nWorld * -1.0f;   // points from sphere toward OBB
    r.depth = radius - dist;
    return r;
}

}  // namespace

int PhysicsWorld::Add(const RigidBody& body) {
    bodies_.push_back(body);
    return static_cast<int>(bodies_.size()) - 1;
}

void PhysicsWorld::Clear() {
    bodies_.clear();
    initial_.clear();
    collisions_.clear();
}

void PhysicsWorld::CaptureInitialState() {
    initial_ = bodies_;
}

void PhysicsWorld::ResetAll() {
    if (initial_.empty()) return;
    for (size_t i = 0; i < bodies_.size() && i < initial_.size(); ++i) {
        bodies_[i].position   = initial_[i].position;
        bodies_[i].velocity   = initial_[i].velocity;
        bodies_[i].angVel     = initial_[i].angVel;
        bodies_[i].orientation = initial_[i].orientation;
    }
}

void PhysicsWorld::Kick(int idx, Vec3 impulse) {
    if (idx < 0 || idx >= static_cast<int>(bodies_.size())) return;
    if (!bodies_[idx].dynamic) return;
    RigidBody& b = bodies_[idx];
    float invM = 1.0f / b.mass;
    b.velocity.x += impulse.x * invM;
    b.velocity.y += impulse.y * invM;
    b.velocity.z += impulse.z * invM;
    b.awake = true;
}

void PhysicsWorld::Kick(int idx, float x, float y, float z) {
    Kick(idx, Vec3{ x, y, z });
}

void PhysicsWorld::Spin(int idx, Vec3 axisRadiansPerSec) {
    if (idx < 0 || idx >= static_cast<int>(bodies_.size())) return;
    if (!bodies_[idx].dynamic) return;
    bodies_[idx].angVel = axisRadiansPerSec;
    bodies_[idx].awake = true;
}

void PhysicsWorld::AddAngVel(int idx, Vec3 delta) {
    if (idx < 0 || idx >= static_cast<int>(bodies_.size())) return;
    if (!bodies_[idx].dynamic) return;
    bodies_[idx].angVel.x += delta.x;
    bodies_[idx].angVel.y += delta.y;
    bodies_[idx].angVel.z += delta.z;
    bodies_[idx].awake = true;
}

int PhysicsWorld::Step(float dt, float damping, float restitution) {
    if (!enabled_) return 0;
    if (restitution >= 0.0f) restitution_ = restitution;
    collisions_.clear();

    // 1. Integrate (semi-implicit Euler) with damping.
    for (auto& b : bodies_) {
        if (!b.dynamic) continue;
        // Apply gravity as acceleration.
        if (gravityEnabled_) {
            b.velocity.x += gravity_.x * dt;
            b.velocity.y += gravity_.y * dt;
            b.velocity.z += gravity_.z * dt;
        }
        b.position.x += b.velocity.x * dt;
        b.position.y += b.velocity.y * dt;
        b.position.z += b.velocity.z * dt;
        b.velocity.x *= damping;
        b.velocity.y *= damping;
        b.velocity.z *= damping;

        // Damping on angular velocity too.
        b.angVel.x *= damping;
        b.angVel.y *= damping;
        b.angVel.z *= damping;

        IntegrateOrientation(b.orientation, b.angVel, dt);
    }

    // 1a. Drag override: drive dragged body so its anchor follows target.
    if (drag_.active && drag_.body >= 0 && drag_.body < static_cast<int>(bodies_.size())) {
        RigidBody& b = bodies_[drag_.body];
        if (b.dynamic) {
            // Anchor offset in local frame (approx; ignores orientation change
            // during drag, which is fine for the demo).
            Vec3 offset = drag_.anchor - b.position;  // current anchor in world
            Vec3 desired = drag_.anchor - offset;      // where center needs to be
            // We don't actually know target — caller updates anchor.
            // Instead just set velocity so the anchor tracks it.
            // anchor_world_new = center + offset
            // So: v = (new_anchor - (center + offset)) / dt
            // We have the delta prevAnchor -> anchor in UpdateDragAnchor.
            Vec3 delta = drag_.anchor - drag_.prevAnchor;
            if (dt > 1e-6f) {
                b.velocity.x = delta.x / dt;
                b.velocity.y = delta.y / dt;
                b.velocity.z = delta.z / dt;
                b.angVel = { 0, 0, 0 };
            }
        }
    }

    // 1b. Refresh AABBs for broadphase.
    aabbs_.clear();
    aabbs_.reserve(bodies_.size());
    for (const auto& b : bodies_) {
        if (b.shape == ShapeKind::Sphere) {
            aabbs_.push_back(AABB::FromSphere(b.position, b.radius));
        } else {
            Vec3 axes[3] = {
                b.orientation.Rotate({ 1, 0, 0 }),
                b.orientation.Rotate({ 0, 1, 0 }),
                b.orientation.Rotate({ 0, 0, 1 })
            };
            aabbs_.push_back(AABB::FromOBB(b.position, axes, b.halfExtents));
        }
    }

    // 2. Collision detection using AABB broadphase.
    int resolved = 0;
    auto pairs = ComputeBroadphasePairs();
    for (const auto& p : pairs) {
        int i = p.first, j = p.second;
        RigidBody& A = bodies_[i];
        RigidBody& B = bodies_[j];
        if (!A.dynamic && !B.dynamic) continue;

            SatHit hit{ false, {0,1,0}, 0.0f, (A.position+B.position)*0.5f };
            if (A.shape == ShapeKind::Sphere && B.shape == ShapeKind::Sphere) {
                Vec3 d = B.position - A.position;
                float d2 = d.x*d.x + d.y*d.y + d.z*d.z;
                float r = A.radius + B.radius;
                if (d2 >= r * r) continue;
                float dist = std::sqrt(std::max(d2, 1e-12f));
                Vec3 nrm = dist > 1e-6f ? d * (1.0f / dist) : Vec3{ 1, 0, 0 };
                hit.hit   = true;
                hit.axis  = nrm;          // from A toward B
                hit.depth = r - dist;
                hit.contact = A.position + nrm * (A.radius);
            } else if (A.shape == ShapeKind::Box && B.shape == ShapeKind::Box) {
                hit = OBB_vs_OBB(MakeOBB(A), MakeOBB(B));
                if (!hit.hit) continue;
            } else if (A.shape == ShapeKind::Sphere && B.shape == ShapeKind::Box) {
                hit = OBB_vs_Sphere(MakeOBB(B), A.position, A.radius);
                if (!hit.hit) continue;
                // hit.axis points from sphere toward OBB; flip so A->B.
                hit.axis = hit.axis * -1.0f;
            } else {
                // Box vs Sphere — symmetric to above.
                hit = OBB_vs_Sphere(MakeOBB(A), B.position, B.radius);
                if (!hit.hit) continue;
                // hit.axis already points from sphere B toward OBB A => A->B.
            }

            CollisionInfo info;
            info.a = i; info.b = j;
            info.normal      = hit.axis;
            info.penetration = hit.depth;
            info.contact     = hit.contact;

            // Inverse masses.
            float invMa = A.dynamic ? 1.0f / A.mass : 0.0f;
            float invMb = B.dynamic ? 1.0f / B.mass : 0.0f;
            float invSum = invMa + invMb;
            if (invSum <= 0.0f) {
                collisions_.push_back(info);
                resolved++;
                continue;
            }

            // Positional correction.
            float corr = info.penetration / invSum;
            Vec3 corrA = info.normal * (-corr * invMa);
            Vec3 corrB = info.normal * ( corr * invMb);
            A.position = A.position + corrA;
            B.position = B.position + corrB;

            // Linear impulse along normal.
            Vec3 rv = B.velocity - A.velocity;
            float vAlong = Dot(rv, info.normal);
            float jLin = 0.0f;
            if (vAlong < 0.0f) {
                jLin = -(1.0f + restitution_) * vAlong / invSum;
                if (A.dynamic) A.velocity = A.velocity + info.normal * (-jLin * invMa);
                if (B.dynamic) B.velocity = B.velocity + info.normal * ( jLin * invMb);
            }

            // Angular impulse: r × J (approximate, treat bodies as point mass with
            // an isotropic inertia proportional to mass).  This gives a visible
            // "spin" when boxes get hit, which is what V0.11 wants to demo.
            // r = contact - center
            if (jLin != 0.0f) {
                Vec3 rA = info.contact - A.position;
                Vec3 rB = info.contact - B.position;
                Vec3 J  = info.normal * jLin;
                if (A.dynamic) A.angVel = A.angVel + Cross(rA, J) * (0.5f * invMa);
                if (B.dynamic) B.angVel = B.angVel + Cross(rB, J) * (0.5f * invMb);
            }

            collisions_.push_back(info);
            resolved++;
    }
    return resolved;
}

// --- Broadphase, picking, selection ----------------------------------------

AABB PhysicsWorld::GetAABB(int i) const {
    if (i < 0 || i >= static_cast<int>(bodies_.size())) return {};
    const RigidBody& b = bodies_[i];
    if (b.shape == ShapeKind::Sphere) return AABB::FromSphere(b.position, b.radius);
    Vec3 axes[3] = {
        b.orientation.Rotate({ 1, 0, 0 }),
        b.orientation.Rotate({ 0, 1, 0 }),
        b.orientation.Rotate({ 0, 0, 1 })
    };
    return AABB::FromOBB(b.position, axes, b.halfExtents);
}

std::vector<std::pair<int,int>> PhysicsWorld::ComputeBroadphasePairs() const {
    std::vector<std::pair<int,int>> pairs;
    const int n = static_cast<int>(bodies_.size());
    if (n < 2) return pairs;

    // Build AABBs on the fly if Step hasn't run yet.
    std::vector<AABB> local;
    const AABB* aabbs = aabbs_.data();
    size_t aabbCount = aabbs_.size();
    if (aabbCount != bodies_.size()) {
        local.reserve(n);
        for (const auto& b : bodies_) {
            if (b.shape == ShapeKind::Sphere) {
                local.push_back(AABB::FromSphere(b.position, b.radius));
            } else {
                Vec3 axes[3] = {
                    b.orientation.Rotate({ 1, 0, 0 }),
                    b.orientation.Rotate({ 0, 1, 0 }),
                    b.orientation.Rotate({ 0, 0, 1 })
                };
                local.push_back(AABB::FromOBB(b.position, axes, b.halfExtents));
            }
        }
        aabbs = local.data();
        aabbCount = local.size();
    }

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (aabbs[i].Overlaps(aabbs[j])) {
                pairs.push_back({ i, j });
            }
        }
    }
    return pairs;
}

void PhysicsWorld::SetSelected(int idx) {
    if (idx >= -1 && idx < static_cast<int>(bodies_.size())) selected_ = idx;
    // End any active drag if we deselect.
    if (selected_ < 0) drag_ = Drag{};
}

bool PhysicsWorld::BeginDrag(int idx, Vec3 anchorWorld) {
    if (idx < 0 || idx >= static_cast<int>(bodies_.size())) return false;
    if (!bodies_[idx].dynamic) return false;
    drag_.active  = true;
    drag_.body    = idx;
    drag_.anchor  = anchorWorld;
    drag_.prevAnchor = anchorWorld;
    selected_     = idx;
    return true;
}

void PhysicsWorld::UpdateDragAnchor(Vec3 anchorWorld) {
    if (!drag_.active) return;
    drag_.prevAnchor = drag_.anchor;
    drag_.anchor     = anchorWorld;
}

void PhysicsWorld::EndDrag() {
    drag_ = Drag{};
}

namespace {

// Ray vs sphere: returns nearest t >= 0 or -1.
float Ray_Sphere(const Ray& r, const Vec3& c, float radius) {
    Vec3 oc = r.origin - c;
    float b = Dot(oc, r.dir);
    float cc = Dot(oc, oc) - radius * radius;
    float disc = b * b - cc;
    if (disc < 0.0f) return -1.0f;
    float t = -b - std::sqrt(disc);
    return (t >= 0.0f) ? t : -1.0f;
}

// Ray vs OBB (slab method in local space).
float Ray_OBB(const Ray& r, const Vec3& center, const Vec3 axes[3], const Vec3& half) {
    // Transform ray to OBB local space.
    Vec3 d_local{
        Dot(r.dir, axes[0]),
        Dot(r.dir, axes[1]),
        Dot(r.dir, axes[2])
    };
    Vec3 oc = r.origin - center;
    Vec3 o_local{
        Dot(oc, axes[0]),
        Dot(oc, axes[1]),
        Dot(oc, axes[2])
    };
    float tmin = -1e30f, tmax = 1e30f;
    for (int i = 0; i < 3; ++i) {
        float o = (&o_local.x)[i];
        float d = (&d_local.x)[i];
        float h = (&half.x)[i];
        if (std::fabs(d) < 1e-6f) {
            if (o < -h || o > h) return -1.0f;
        } else {
            float t1 = (-h - o) / d;
            float t2 = ( h - o) / d;
            if (t1 > t2) std::swap(t1, t2);
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return -1.0f;
        }
    }
    if (tmin < 0.0f) tmin = 0.0f;
    return tmin;
}

}  // namespace

RayHit PhysicsWorld::RayCast(const Ray& r, float maxDist) const {
    RayHit best;
    best.t = maxDist;
    for (int i = 0; i < static_cast<int>(bodies_.size()); ++i) {
        const RigidBody& b = bodies_[i];
        float t = -1.0f;
        if (b.shape == ShapeKind::Sphere) {
            t = Ray_Sphere(r, b.position, b.radius);
        } else {
            Vec3 axes[3] = {
                b.orientation.Rotate({ 1, 0, 0 }),
                b.orientation.Rotate({ 0, 1, 0 }),
                b.orientation.Rotate({ 0, 0, 1 })
            };
            t = Ray_OBB(r, b.position, axes, b.halfExtents);
        }
        if (t >= 0.0f && t < best.t) {
            best.body = i;
            best.t    = t;
            best.point = r.origin + r.dir * t;
        }
    }
    if (best.body < 0) {
        best.t = 0.0f;
    }
    return best;
}

}  // namespace xe

