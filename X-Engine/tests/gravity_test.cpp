#include <gtest/gtest.h>
#include "physics/physics_world.h"

namespace {

TEST(GravityTest, DefaultZero) {
    xe::PhysicsWorld w;
    auto g = w.Gravity();
    EXPECT_FLOAT_EQ(g.x, 0.0f);
    EXPECT_FLOAT_EQ(g.y, 0.0f);
    EXPECT_FLOAT_EQ(g.z, 0.0f);
    EXPECT_FALSE(w.GravityEnabled());
}

TEST(GravityTest, SetAndGet) {
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    EXPECT_FLOAT_EQ(w.Gravity().y, -9.81f);
}

TEST(GravityTest, DisabledNoEffect) {
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    w.SetGravityEnabled(false);
    xe::RigidBody b;
    b.position = { 0, 10, 0 };
    w.Add(b);
    w.Step(1.0f);
    EXPECT_FLOAT_EQ(w.Get(0).velocity.y, 0.0f);
}

TEST(GravityTest, EnabledAcceleratesDown) {
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    w.SetGravityEnabled(true);
    xe::RigidBody b;
    b.position = { 0, 10, 0 };
    w.Add(b);
    w.Step(1.0f);
    EXPECT_LT(w.Get(0).velocity.y, -9.0f);
}

TEST(GravityTest, StaticBodyUnaffectedByGravity) {
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    w.SetGravityEnabled(true);
    xe::RigidBody b;
    b.dynamic = false;
    b.position = { 0, 10, 0 };
    w.Add(b);
    w.Step(1.0f);
    EXPECT_FLOAT_EQ(w.Get(0).velocity.y, 0.0f);
}

TEST(GravityTest, FreeFall1Sec) {
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    w.SetGravityEnabled(true);
    xe::RigidBody b;
    b.position = { 0, 0, 0 };
    w.Add(b);
    for (int i = 0; i < 60; ++i) w.Step(1.0f / 60.0f);
    auto& body = w.Get(0);
    // With damping, after 1s the body is clearly below origin but not at -4.9m.
    EXPECT_LT(body.position.y, -3.0f);
    EXPECT_GT(body.position.y, -5.5f);
}

// --- Rotation drag / AddAngVel --------------------------------------------

TEST(RotDragTest, AddAngVelAccumulates) {
    xe::PhysicsWorld w;
    xe::RigidBody b; w.Add(b);
    w.AddAngVel(0, { 0, 1.0f, 0 });
    w.AddAngVel(0, { 0, 0.5f, 0 });
    EXPECT_NEAR(w.Get(0).angVel.y, 1.5f, 1e-5f);
}

TEST(RotDragTest, AddAngVelInvalidIndexIgnored) {
    xe::PhysicsWorld w;
    w.AddAngVel(5, { 1, 0, 0 });  // no crash
    SUCCEED();
}

TEST(RotDragTest, AddAngVelStaticIgnored) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.dynamic = false;
    w.Add(b);
    w.AddAngVel(0, { 1, 0, 0 });
    EXPECT_FLOAT_EQ(w.Get(0).angVel.x, 0.0f);
}

TEST(RotDragTest, RotationDragChangesOrientationOverTime) {
    // Simulate 60 frames where we add small angVel each tick.
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.shape = xe::ShapeKind::Box;
    b.halfExtents = { 0.5f, 0.5f, 0.5f };
    w.Add(b);
    for (int i = 0; i < 60; ++i) {
        w.AddAngVel(0, { 0, 1.0f / 60.0f, 0 });   // ~1 rad/s about Y
        w.Step(1.0f / 60.0f);
    }
    auto worldX = w.Get(0).orientation.Rotate({ 1, 0, 0 });
    // After ~1s (with damping maybe ~0.7 rad), the X axis should be rotated.
    EXPECT_LT(worldX.z, -0.3f);
    EXPECT_LT(worldX.x,  0.95f);
}

}  // namespace
