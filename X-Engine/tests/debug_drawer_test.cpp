#include <gtest/gtest.h>
#include "debug/debug_drawer.h"
#include "physics/physics_world.h"

namespace {

TEST(DebugDrawTest, HiddenProducesNoLines) {
    xe::DebugDrawer d;
    d.SetVisible(false);
    xe::PhysicsWorld w;
    w.AddSphere({ 0, 0, 0 }, 1.0f);
    d.Rebuild(w, xe::Mat4::Identity());
    EXPECT_TRUE(d.Lines().empty());
}

TEST(DebugDrawTest, AABBVisibleProducesLines) {
    xe::DebugDrawer d;
    d.SetShowAABBs(true);
    xe::PhysicsWorld w;
    w.AddSphere({ 0, 0, 0 }, 1.0f);
    d.Rebuild(w, xe::Mat4::Identity());
    EXPECT_FALSE(d.Lines().empty());
    // 12 edges for an AABB.
    EXPECT_EQ(d.Lines().size(), 12u);
}

TEST(DebugDrawTest, TwoAABBs) {
    xe::DebugDrawer d;
    d.SetShowAABBs(true);
    xe::PhysicsWorld w;
    w.AddSphere({ 0, 0, 0 }, 1.0f);
    w.AddSphere({ 5, 0, 0 }, 1.0f);
    d.Rebuild(w, xe::Mat4::Identity());
    EXPECT_EQ(d.Lines().size(), 24u);
}

TEST(DebugDrawTest, TriggerWireframeVisible) {
    xe::DebugDrawer d;
    d.SetShowTriggers(true);
    d.SetShowAABBs(false);
    d.SetShowJoints(false);
    xe::PhysicsWorld w;
    xe::RigidBody t;
    t.shape = xe::ShapeKind::Sphere;
    t.isTrigger = true;
    t.dynamic = false;
    t.radius = 1.0f;
    w.Add(t);
    d.Rebuild(w, xe::Mat4::Identity());
    // 3 great circles, 16 segments each = 48 lines.
    EXPECT_EQ(d.Lines().size(), 48u);
}

TEST(DebugDrawTest, JointLineVisible) {
    xe::DebugDrawer d;
    d.SetShowJoints(true);
    d.SetShowAABBs(false);
    d.SetShowTriggers(false);
    xe::PhysicsWorld w;
    w.AddSphere({ 0, 0, 0 }, 0.5f);
    w.AddSphere({ 1, 0, 0 }, 0.5f);
    xe::BallJoint j; j.a = 0; j.b = 1; w.AddBallJoint(j);
    d.Rebuild(w, xe::Mat4::Identity());
    EXPECT_GE(d.Lines().size(), 1u);
}

TEST(DebugDrawTest, VelocityVisible) {
    xe::DebugDrawer d;
    d.SetShowVelocity(true);
    d.SetShowAABBs(false);
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.velocity = { 1, 0, 0 };
    b.dynamic = true;
    w.Add(b);
    d.Rebuild(w, xe::Mat4::Identity());
    EXPECT_EQ(d.Lines().size(), 1u);
    EXPECT_GT(d.Lines()[0].b.x, 0.0f);
}

TEST(DebugDrawTest, ToggleAll) {
    xe::DebugDrawer d;
    d.ToggleAll();
    EXPECT_TRUE(d.ShowAABBs());
    EXPECT_TRUE(d.ShowJoints());
    d.ToggleAll();
    EXPECT_FALSE(d.ShowAABBs());
    EXPECT_FALSE(d.ShowJoints());
}

TEST(DebugDrawTest, BoxOBBLines) {
    xe::DebugDrawer d;
    d.SetShowAABBs(true);
    xe::PhysicsWorld w;
    w.AddBox({ 0, 0, 0 }, { 0.5f, 0.5f, 0.5f });
    d.Rebuild(w, xe::Mat4::Identity());
    // AABB of an axis-aligned 1x1x1 box centered at origin: 12 edges.
    EXPECT_EQ(d.Lines().size(), 12u);
}

}  // namespace
