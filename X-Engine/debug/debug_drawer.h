#pragma once

#include "physics/physics_world.h"
#include "core/math.h"
#include <vector>

namespace xe {

// DebugDrawer: builds a list of world-space line segments representing
// physics state for visualization (AABBs, joint anchors, trigger outlines,
// contact normals, etc.).  Independent of any graphics API: the caller
// projects and renders the lines.
class DebugDrawer {
public:
    struct LineSegment {
        Vec3 a;
        Vec3 b;
        Vec3 color;  // 0..1 RGB
    };

    void SetVisible(bool v) { visible_ = v; }
    bool Visible() const     { return visible_; }

    // Build the segments for one frame.  Pass a 4x4 view-projection matrix
    // (row-major, like Mat4) so we can do a per-line near-plane cull.
    void Rebuild(const PhysicsWorld& w, const Mat4& viewProj);

    const std::vector<LineSegment>& Lines() const { return lines_; }
    void Clear() { lines_.clear(); }

    // Toggle which kinds of debug visualization to draw.
    bool ShowAABBs()      const { return showAABB_; }
    bool ShowJoints()     const { return showJoints_; }
    bool ShowTriggers()   const { return showTriggers_; }
    bool ShowContacts()   const { return showContacts_; }
    bool ShowVelocity()   const { return showVelocity_; }
    void SetShowAABBs(bool b)    { showAABB_    = b; }
    void SetShowJoints(bool b)   { showJoints_  = b; }
    void SetShowTriggers(bool b) { showTriggers_= b; }
    void SetShowContacts(bool b) { showContacts_= b; }
    void SetShowVelocity(bool b) { showVelocity_= b; }
    void ToggleAll() {
        bool all = showAABB_ && showJoints_ && showTriggers_ && showContacts_ && showVelocity_;
        bool v = !all;
        showAABB_ = showJoints_ = showTriggers_ = showContacts_ = showVelocity_ = v;
    }

private:
    bool visible_ = true;
    bool showAABB_    = false;
    bool showJoints_  = false;
    bool showTriggers_= true;
    bool showContacts_= false;
    bool showVelocity_= false;
    std::vector<LineSegment> lines_;
};

}  // namespace xe
