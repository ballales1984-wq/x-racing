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
    bool  awake    = true;   // false = sleeping (not integrated)
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

// Distance constraint: keeps two bodies at a target rest length.
struct DistanceConstraint {
    int   a      = -1;
    int   b      = -1;
    float restLength = 0.0f;
    float stiffness  = 1.0f;  // 0 = no pull, 1 = rigid
};

class PhysicsWorld {
public:
    PhysicsWorld() = default;

    int  Add(const RigidBody& body);
    void Clear();

    // Step the simulation. Returns number of collisions resolved.
    int Step(float dt, float damping = 0.999f, float restitution = 0.5f);

    // ---- Sleeping (V0.15) ---------------------------------------------
    void   SetSleepLinearThreshold(float t)  { sleepLinear_  = t; }
    void   SetSleepAngularThreshold(float t) { sleepAngular_ = t; }
    void   SetSleepTimeRequired(float s)     { sleepTime_    = s; }
    float  SleepLinearThreshold() const      { return sleepLinear_; }
    float  SleepAngularThreshold() const     { return sleepAngular_; }
    float  SleepTimeRequired() const         { return sleepTime_; }
    bool   SleepingEnabled() const           { return sleepEnabled_; }
    void   SetSleepingEnabled(bool e)        { sleepEnabled_ = e; }
    int    SleepBody(int idx);
    int    WakeBody(int idx);
    int    SleepingCount() const;

    // Apply linear impulse.
    void Kick(int idx, Vec3 impulse);
    void Kick(int idx, float x, float y, float z);

    // Apply angular impulse (tweak angVel directly — for the "spin" demo).
    void Spin(int idx, Vec3 axisRadiansPerSec);

    // Add to existing angular velocity (for interactive rotation drag).
    void AddAngVel(int idx, Vec3 delta);

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

    // ---- Drag state (for V0.13 interactive editing) -------------------
    // Begin dragging a body at a given world-space anchor point (typically
    // the pick point on the body). Returns false if idx invalid or static.
    bool BeginDrag(int idx, Vec3 anchorWorld);

    // Update the anchor of the currently dragged body each frame.
    void UpdateDragAnchor(Vec3 anchorWorld);

    // Stop dragging.  If `applyImpulse`, the relative velocity is preserved
    // (the body's velocity already follows the anchor's velocity each step).
    void EndDrag();

    bool   IsDragging() const  { return drag_.active; }
    int    DragIndex() const   { return drag_.body; }
    Vec3   DragAnchor() const  { return drag_.anchor; }

    // ---- Constraints (V0.15) -------------------------------------------
    int  AddConstraint(const DistanceConstraint& c);
    void RemoveConstraint(int cid);
    void ClearConstraints() { constraints_.clear(); }
    int  NumConstraints() const { return static_cast<int>(constraints_.size()); }
    const DistanceConstraint& GetConstraint(int cid) const { return constraints_[cid]; }

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

    // Gravity (acceleration applied to dynamic bodies each step). Default = (0,0,0).
    void   SetGravity(Vec3 g) { gravity_ = g; }
    Vec3   Gravity() const    { return gravity_; }
    void   SetGravityEnabled(bool e) { gravityEnabled_ = e; }
    bool   GravityEnabled() const    { return gravityEnabled_; }

private:
    struct Drag {
        bool  active  = false;
        int   body    = -1;
        Vec3  anchor{ 0,0,0 };   // world point on the body we grabbed
        Vec3  prevAnchor{ 0,0,0 };
    };
    std::vector<RigidBody>        bodies_;
    std::vector<RigidBody>        initial_;
    std::vector<CollisionInfo>    collisions_;
    std::vector<AABB>             aabbs_;       // parallel to bodies_
    bool enabled_    = true;
    float restitution_ = 0.5f;
    int  selected_  = -1;
    Drag drag_;
    Vec3 gravity_       = { 0, 0, 0 };
    bool gravityEnabled_ = false;
    float sleepLinear_  = 0.05f;
    float sleepAngular_ = 0.10f;
    float sleepTime_    = 0.5f;
    bool  sleepEnabled_ = true;
    std::vector<float>     sleepAccum_;   // per-body time below threshold
    std::vector<DistanceConstraint> constraints_;
};

}  // namespace xe
