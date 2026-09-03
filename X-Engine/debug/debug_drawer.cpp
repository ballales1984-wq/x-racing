#include "debug/debug_drawer.h"
#include <cmath>

namespace xe {

namespace {

void AddBoxLines(std::vector<DebugDrawer::LineSegment>& out,
                  const Vec3& mn, const Vec3& mx, const Vec3& color) {
    // 12 edges of an AABB.
    Vec3 c[8] = {
        { mn.x, mn.y, mn.z }, { mx.x, mn.y, mn.z },
        { mx.x, mx.y, mn.z }, { mn.x, mx.y, mn.z },
        { mn.x, mn.y, mx.z }, { mx.x, mn.y, mx.z },
        { mx.x, mx.y, mx.z }, { mn.x, mx.y, mx.z },
    };
    int e[12][2] = {
        {0,1},{1,2},{2,3},{3,0},   // bottom
        {4,5},{5,6},{6,7},{7,4},   // top
        {0,4},{1,5},{2,6},{3,7},   // vertical
    };
    for (auto& i : e) out.push_back({ c[i[0]], c[i[1]], color });
}

void AddOBBLines(std::vector<DebugDrawer::LineSegment>& out,
                  const Vec3& center, const Vec3 axes[3], const Vec3& half,
                  const Vec3& color) {
    Vec3 e[3] = { axes[0]*half.x, axes[1]*half.y, axes[2]*half.z };
    Vec3 c[8];
    int idx = 0;
    for (int sx = -1; sx <= 1; sx += 2)
    for (int sy = -1; sy <= 1; sy += 2)
    for (int sz = -1; sz <= 1; sz += 2) {
        c[idx++] = center + e[0]*(float)sx + e[1]*(float)sy + e[2]*(float)sz;
    }
    int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7},
    };
    for (auto& i : edges) out.push_back({ c[i[0]], c[i[1]], color });
}

Vec3 SphereAABB(const RigidBody& b) {
    return { b.radius, b.radius, b.radius };
}

}  // namespace

void DebugDrawer::Rebuild(const PhysicsWorld& w, const Mat4& viewProj) {
    lines_.clear();
    if (!visible_) return;

    (void)viewProj;  // future: per-line cull

    const Vec3 colAABB    = { 0.2f, 0.6f, 1.0f };
    const Vec3 colJoint   = { 1.0f, 1.0f, 0.2f };
    const Vec3 colTrigger = { 1.0f, 0.2f, 1.0f };
    const Vec3 colContact = { 1.0f, 0.4f, 0.0f };
    const Vec3 colVel     = { 0.2f, 1.0f, 0.4f };

    // AABBs
    if (showAABB_) {
        for (int i = 0; i < w.Size(); ++i) {
            AABB a = w.GetAABB(i);
            AddBoxLines(lines_, a.min, a.max, colAABB);
        }
    }

    // Joint anchors + lines
    if (showJoints_) {
        for (int i = 0; i < w.NumBallJoints(); ++i) {
            const BallJoint& j = w.GetBallJoint(i);
            if (j.broken) continue;
            if (j.a < 0 || j.b < 0) continue;
            if (j.a >= w.Size() || j.b >= w.Size()) continue;
            const RigidBody& A = w.Get(j.a);
            const RigidBody& B = w.Get(j.b);
            Vec3 wa = A.position + A.orientation.Rotate(j.localA);
            Vec3 wb = B.position + B.orientation.Rotate(j.localB);
            lines_.push_back({ wa, wb, colJoint });
        }
        for (int i = 0; i < w.NumHingeJoints(); ++i) {
            const HingeJoint& j = w.GetHingeJoint(i);
            if (j.broken) continue;
            if (j.a < 0 || j.b < 0) continue;
            if (j.a >= w.Size() || j.b >= w.Size()) continue;
            const RigidBody& A = w.Get(j.a);
            const RigidBody& B = w.Get(j.b);
            Vec3 wa = A.position + A.orientation.Rotate(j.localA);
            Vec3 wb = B.position + B.orientation.Rotate(j.localB);
            lines_.push_back({ wa, wb, colJoint });
            // Hinge axis as a short line on A.
            Vec3 axisEnd = wa + A.orientation.Rotate(j.localAxisA) * 0.6f;
            lines_.push_back({ wa, axisEnd, { 0.2f, 0.2f, 1.0f } });
        }
    }

    // Trigger outlines (in their own bright color)
    if (showTriggers_) {
        for (int i = 0; i < w.Size(); ++i) {
            const RigidBody& b = w.Get(i);
            if (!b.isTrigger) continue;
            if (b.shape == ShapeKind::Sphere) {
                // 3-axis-aligned great circles (a wireframe sphere).
                Vec3 c = b.position;
                float r = b.radius;
                int N = 16;
                for (int axis = 0; axis < 3; ++axis) {
                    for (int j = 0; j < N; ++j) {
                        float t0 = (float)j / N * 6.2831853f;
                        float t1 = (float)(j+1) / N * 6.2831853f;
                        Vec3 p0, p1;
                        if (axis == 0) { p0 = { c.x, c.y + r*std::cos(t0), c.z + r*std::sin(t0) };
                                          p1 = { c.x, c.y + r*std::cos(t1), c.z + r*std::sin(t1) }; }
                        else if (axis == 1) { p0 = { c.x + r*std::cos(t0), c.y, c.z + r*std::sin(t0) };
                                               p1 = { c.x + r*std::cos(t1), c.y, c.z + r*std::sin(t1) }; }
                        else { p0 = { c.x + r*std::cos(t0), c.y + r*std::sin(t0), c.z };
                               p1 = { c.x + r*std::cos(t1), c.y + r*std::sin(t1), c.z }; }
                        lines_.push_back({ p0, p1, colTrigger });
                    }
                }
            } else {
                Vec3 axes[3] = {
                    b.orientation.Rotate({ 1, 0, 0 }),
                    b.orientation.Rotate({ 0, 1, 0 }),
                    b.orientation.Rotate({ 0, 0, 1 })
                };
                AddOBBLines(lines_, b.position, axes, b.halfExtents, colTrigger);
            }
        }
    }

    // Contact normals
    if (showContacts_) {
        for (const auto& c : w.LastCollisions()) {
            Vec3 base = c.contact;
            Vec3 end  = { base.x + c.normal.x * 0.5f,
                          base.y + c.normal.y * 0.5f,
                          base.z + c.normal.z * 0.5f };
            lines_.push_back({ base, end, colContact });
        }
    }

    // Velocity vectors
    if (showVelocity_) {
        for (int i = 0; i < w.Size(); ++i) {
            const RigidBody& b = w.Get(i);
            if (!b.dynamic) continue;
            Vec3 end = { b.position.x + b.velocity.x,
                         b.position.y + b.velocity.y,
                         b.position.z + b.velocity.z };
            lines_.push_back({ b.position, end, colVel });
        }
    }
}

}  // namespace xe
