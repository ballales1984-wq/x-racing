#include <gtest/gtest.h>
#include "physics/physics_world.h"

namespace {

// --- Sleeping --------------------------------------------------------------

TEST(SleepTest, DefaultEnabled) {
    xe::PhysicsWorld w;
    EXPECT_TRUE(w.SleepingEnabled());
}

TEST(SleepTest, QuiescentBodySleeps) {
    xe::PhysicsWorld w;
    w.SetSleepTimeRequired(0.1f);
    w.SetGravityEnabled(false);
    xe::RigidBody b;
    b.position = { 0, 0, 0 };
    b.velocity = { 0.001f, 0, 0 };  // below threshold
    w.Add(b);
    // Step 200ms total in 1ms steps.
    for (int i = 0; i < 200; ++i) w.Step(0.001f);
    EXPECT_FALSE(w.Get(0).awake);
    EXPECT_GE(w.SleepingCount(), 1);
}

TEST(SleepTest, MovingBodyStaysAwake) {
    xe::PhysicsWorld w;
    w.SetSleepTimeRequired(0.1f);
    w.SetGravityEnabled(false);
    xe::RigidBody b;
    b.position = { 0, 0, 0 };
    b.velocity = { 5, 0, 0 };  // well above threshold
    w.Add(b);
    for (int i = 0; i < 200; ++i) w.Step(0.001f);
    EXPECT_TRUE(w.Get(0).awake);
}

TEST(SleepTest, SleepBodyManual) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.velocity = { 5, 0, 0 };
    w.Add(b);
    w.SleepBody(0);
    EXPECT_FALSE(w.Get(0).awake);
    EXPECT_FLOAT_EQ(w.Get(0).velocity.x, 0.0f);
}

TEST(SleepTest, WakeBodyManual) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    w.Add(b);
    w.SleepBody(0);
    w.WakeBody(0);
    EXPECT_TRUE(w.Get(0).awake);
}

TEST(SleepTest, SleepingBodySkipsIntegration) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.position = { 0, 0, 0 };
    w.Add(b);
    w.SleepBody(0);
    // Even with gravity, the body should not move.
    w.SetGravityEnabled(true);
    w.SetGravity({ 0, -9.81f, 0 });
    for (int i = 0; i < 60; ++i) w.Step(0.016f);
    EXPECT_FLOAT_EQ(w.Get(0).position.y, 0.0f);
}

TEST(SleepTest, DisabledNoSleep) {
    xe::PhysicsWorld w;
    w.SetSleepingEnabled(false);
    w.SetSleepTimeRequired(0.01f);
    xe::RigidBody b;
    b.position = { 0, 0, 0 };
    b.velocity = { 0.001f, 0, 0 };
    w.Add(b);
    for (int i = 0; i < 60; ++i) w.Step(0.016f);
    EXPECT_TRUE(w.Get(0).awake);
}

// --- Distance constraints --------------------------------------------------

TEST(ConstraintTest, AddReturnsValidId) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b; w.Add(a); w.Add(b);
    xe::DistanceConstraint c;
    c.a = 0; c.b = 1; c.restLength = 1.0f;
    int id = w.AddConstraint(c);
    EXPECT_GE(id, 0);
    EXPECT_EQ(w.NumConstraints(), 1);
}

TEST(ConstraintTest, EnforcesRestLength) {
    // Two bodies, constraint at 1.0.  Start 2.0 apart.
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody a, b;
    a.position = { 0, 0, 0 };
    b.position = { 2, 0, 0 };
    a.mass = 1; b.mass = 1;
    w.Add(a); w.Add(b);
    xe::DistanceConstraint c;
    c.a = 0; c.b = 1; c.restLength = 1.0f; c.stiffness = 1.0f;
    w.AddConstraint(c);
    // After one Step, both bodies should move toward 1.0 distance.
    w.Step(0.016f);
    auto& A = w.Get(0);
    auto& B = w.Get(1);
    float d = std::sqrt((A.position.x-B.position.x)*(A.position.x-B.position.x) +
                         (A.position.y-B.position.y)*(A.position.y-B.position.y) +
                         (A.position.z-B.position.z)*(A.position.z-B.position.z));
    // Constraint is position-based, so after one frame it should be very close to 1.0.
    EXPECT_NEAR(d, 1.0f, 0.05f);
}

TEST(ConstraintTest, AsymmetricMass) {
    // Heavy static, light dynamic: distance fixed.
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody A, B;
    A.position = { 0, 0, 0 }; A.mass = 1.0f;   // dynamic
    B.position = { 0, 5, 0 }; B.mass = 1.0f; B.dynamic = false;  // anchor
    w.Add(A); w.Add(B);
    xe::DistanceConstraint c;
    c.a = 0; c.b = 1; c.restLength = 2.0f; c.stiffness = 1.0f;
    w.AddConstraint(c);
    w.Step(0.016f);
    auto& a = w.Get(0);
    float dy = a.position.y;
    // dynamic body should have moved toward y=3 (distance 2 from B at y=5).
    EXPECT_NEAR(dy, 3.0f, 0.05f);
}

TEST(ConstraintTest, RemoveConstraint) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b; w.Add(a); w.Add(b);
    xe::DistanceConstraint c; c.a = 0; c.b = 1; c.restLength = 1.0f;
    int id = w.AddConstraint(c);
    w.RemoveConstraint(id);
    EXPECT_EQ(w.NumConstraints(), 0);
}

TEST(ConstraintTest, AddInvalidConstraintReturnsMinusOne) {
    xe::PhysicsWorld w;
    xe::RigidBody a; w.Add(a);
    xe::DistanceConstraint c; c.a = 0; c.b = 99; c.restLength = 1.0f;
    EXPECT_EQ(w.AddConstraint(c), -1);
}

TEST(ConstraintTest, ClearConstraints) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b; w.Add(a); w.Add(b);
    xe::DistanceConstraint c; c.a = 0; c.b = 1; c.restLength = 1.0f;
    w.AddConstraint(c);
    w.AddConstraint(c);
    EXPECT_EQ(w.NumConstraints(), 2);
    w.ClearConstraints();
    EXPECT_EQ(w.NumConstraints(), 0);
}

TEST(ConstraintTest, WakesSleepingBodies) {
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody a, b;
    a.mass = 1.0f; b.mass = 1.0f;
    w.Add(a); w.Add(b);
    w.SleepBody(0); w.SleepBody(1);
    xe::DistanceConstraint c; c.a = 0; c.b = 1; c.restLength = 1.0f;
    w.AddConstraint(c);
    w.Step(0.016f);
    EXPECT_TRUE(w.Get(0).awake);
    EXPECT_TRUE(w.Get(1).awake);
}

}  // namespace
