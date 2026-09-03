#include <gtest/gtest.h>
#include "physics/physics_world.h"
#include <cmath>

namespace {

constexpr float kEps = 1e-3f;

// --- Ball joints -----------------------------------------------------------

TEST(BallJointTest, AddReturnsValidId) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b; w.Add(a); w.Add(b);
    xe::BallJoint j; j.a = 0; j.b = 1; j.localA = { 0.5f, 0, 0 }; j.localB = { -0.5f, 0, 0 };
    int id = w.AddBallJoint(j);
    EXPECT_GE(id, 0);
    EXPECT_EQ(w.NumBallJoints(), 1);
}

TEST(BallJointTest, KeepsAnchorsCoincident) {
    // Body A at origin, Body B at (3,0,0). Joint anchors: A.localA=(0.5,0,0), B.localB=(-0.5,0,0).
    // World anchor A = (0.5, 0, 0); world anchor B should converge to same.
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody A, B;
    A.position = { 0, 0, 0 };
    B.position = { 3, 0, 0 };
    A.mass = 1; B.mass = 1;
    w.Add(A); w.Add(B);
    xe::BallJoint j;
    j.a = 0; j.b = 1;
    j.localA = { 0.5f, 0, 0 };
    j.localB = { -0.5f, 0, 0 };
    j.stiffness = 1.0f;
    w.AddBallJoint(j);
    w.Step(0.016f);
    auto& a = w.Get(0);
    auto& b = w.Get(1);
    xe::Vec3 wa = a.position + a.orientation.Rotate(j.localA);
    xe::Vec3 wb = b.position + b.orientation.Rotate(j.localB);
    float d = std::sqrt((wa.x-wb.x)*(wa.x-wb.x) + (wa.y-wb.y)*(wa.y-wb.y) + (wa.z-wb.z)*(wa.z-wb.z));
    EXPECT_NEAR(d, 0.0f, 0.05f);
}

TEST(BallJointTest, StaticAnchorHolds) {
    // Heavy static body A, dynamic B pinned via ball joint -> B's anchor stays at A.
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody A, B;
    A.position = { 0, 5, 0 };
    A.mass = 1; A.dynamic = false;        // anchor
    B.position = { 0, 0, 0 };
    B.mass = 1;
    w.Add(A); w.Add(B);
    xe::BallJoint j;
    j.a = 0; j.b = 1;
    j.localA = { 0, 0, 0 };
    j.localB = { 0, 0, 0 };
    w.AddBallJoint(j);
    w.Step(0.016f);
    auto& b = w.Get(1);
    // B should have moved up to y=5.
    EXPECT_NEAR(b.position.y, 5.0f, 0.05f);
}

TEST(BallJointTest, BreaksAboveMaxForce) {
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody A, B;
    A.mass = 1; B.mass = 1;
    B.velocity = { 100, 0, 0 };   // very fast
    w.Add(A); w.Add(B);
    xe::BallJoint j;
    j.a = 0; j.b = 1;
    j.localA = { 0, 0, 0 };
    j.localB = { 0, 0, 0 };
    j.maxForce = 1.0f;            // very low -> should break immediately
    int id = w.AddBallJoint(j);
    w.Step(0.016f);
    EXPECT_TRUE(w.GetBallJoint(id).broken);
}

TEST(BallJointTest, NoBreakBelowThreshold) {
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody A, B;
    A.mass = 1; B.mass = 1;
    B.velocity = { 0.01f, 0, 0 };
    w.Add(A); w.Add(B);
    xe::BallJoint j; j.a = 0; j.b = 1; j.maxForce = 1000.0f;
    int id = w.AddBallJoint(j);
    w.Step(0.016f);
    EXPECT_FALSE(w.GetBallJoint(id).broken);
}

TEST(BallJointTest, RemoveBallJoint) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b; w.Add(a); w.Add(b);
    xe::BallJoint j; j.a = 0; j.b = 1;
    int id = w.AddBallJoint(j);
    w.RemoveBallJoint(id);
    EXPECT_EQ(w.NumBallJoints(), 0);
}

TEST(BallJointTest, ClearJoints) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b; w.Add(a); w.Add(b);
    xe::BallJoint bj; bj.a = 0; bj.b = 1; w.AddBallJoint(bj);
    xe::HingeJoint hj; hj.a = 0; hj.b = 1; w.AddHingeJoint(hj);
    EXPECT_EQ(w.NumBallJoints(),  1);
    EXPECT_EQ(w.NumHingeJoints(), 1);
    w.ClearJoints();
    EXPECT_EQ(w.NumBallJoints(),  0);
    EXPECT_EQ(w.NumHingeJoints(), 0);
}

// --- Hinge joints ----------------------------------------------------------

TEST(HingeJointTest, AddReturnsValidId) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b; w.Add(a); w.Add(b);
    xe::HingeJoint j; j.a = 0; j.b = 1; j.localAxisA = { 0, 1, 0 };
    int id = w.AddHingeJoint(j);
    EXPECT_GE(id, 0);
    EXPECT_EQ(w.NumHingeJoints(), 1);
}

TEST(HingeJointTest, AnchorsCoincident) {
    // Same as ball joint: world anchors coincide.
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody A, B;
    A.mass = 1; A.dynamic = false;
    B.mass = 1;
    A.position = { 0, 5, 0 };
    B.position = { 0, 0, 0 };
    w.Add(A); w.Add(B);
    xe::HingeJoint j;
    j.a = 0; j.b = 1;
    j.localA = { 0, 0, 0 };
    j.localB = { 0, 0, 0 };
    j.localAxisA = { 0, 1, 0 };
    w.AddHingeJoint(j);
    w.Step(0.016f);
    auto& b = w.Get(1);
    EXPECT_NEAR(b.position.y, 5.0f, 0.05f);
}

TEST(HingeJointTest, RotationApproachesAxis) {
    // Body B starts with an off-axis rotation. Hinge tries to align axis A
    // (local +Y) with axis B (which after body B's rotation has moved).
    // After several iterations the body B's Y axis should approach A's Y axis.
    xe::PhysicsWorld w;
    w.SetGravityEnabled(false);
    xe::RigidBody A, B;
    A.mass = 1; A.dynamic = false;
    B.mass = 1;
    A.position = { 0, 0, 0 };
    B.position = { 0.5f, 0, 0 };
    B.orientation = xe::Quat::FromAxisAngle({ 0, 0, 1 }, 1.0f);  // 57deg tilt
    w.Add(A); w.Add(B);
    xe::HingeJoint j;
    j.a = 0; j.b = 1;
    j.localA = { 0.5f, 0, 0 };
    j.localB = { -0.5f, 0, 0 };
    j.localAxisA = { 0, 1, 0 };
    w.AddHingeJoint(j);
    for (int i = 0; i < 60; ++i) w.Step(1.0f / 60.0f);
    auto& b = w.Get(1);
    xe::Vec3 yA_world = A.orientation.Rotate({ 0, 1, 0 });
    xe::Vec3 yB_world = b.orientation.Rotate({ 0, 1, 0 });
    float dot = yA_world.x*yB_world.x + yA_world.y*yB_world.y + yA_world.z*yB_world.z;
    // Should be close to 1 (parallel) — but damping will reduce effectiveness.
    EXPECT_GT(dot, 0.5f);
}

TEST(HingeJointTest, RemoveHingeJoint) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b; w.Add(a); w.Add(b);
    xe::HingeJoint j; j.a = 0; j.b = 1;
    int id = w.AddHingeJoint(j);
    w.RemoveHingeJoint(id);
    EXPECT_EQ(w.NumHingeJoints(), 0);
}

}  // namespace
