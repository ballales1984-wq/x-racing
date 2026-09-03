#include <gtest/gtest.h>
#include "physics/physics_world.h"
#include <cmath>

namespace {

constexpr float kEps = 1e-3f;

// --- Quat -------------------------------------------------------------------

TEST(QuatTest, IdentityRotatesZero) {
    auto q = xe::Quat::Identity();
    auto v = q.Rotate({ 1.0f, 0.0f, 0.0f });
    EXPECT_NEAR(v.x, 1.0f, kEps);
    EXPECT_NEAR(v.y, 0.0f, kEps);
    EXPECT_NEAR(v.z, 0.0f, kEps);
}

TEST(QuatTest, AxisAngle90RotatesYtoX) {
    auto q = xe::Quat::FromAxisAngle({ 0, 1, 0 }, 1.5707963f);
    auto v = q.Rotate({ 1, 0, 0 });
    EXPECT_NEAR(v.x,  0.0f, kEps);
    EXPECT_NEAR(v.y,  0.0f, kEps);
    EXPECT_NEAR(v.z, -1.0f, kEps);
}

TEST(QuatTest, ToEulerIdentityIsZero) {
    auto e = xe::Quat::Identity().ToEulerXYZ();
    EXPECT_NEAR(e.x, 0.0f, kEps);
    EXPECT_NEAR(e.y, 0.0f, kEps);
    EXPECT_NEAR(e.z, 0.0f, kEps);
}

TEST(QuatTest, NormalizeStaysUnit) {
    xe::Quat q{ 1, 2, 3, 4 };
    q.Normalize();
    float n2 = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    EXPECT_NEAR(n2, 1.0f, kEps);
}

// --- OBB collision ----------------------------------------------------------

TEST(ObbTest, TwoCubesOverlap) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b;
    a.shape = xe::ShapeKind::Box;
    a.position = { 0, 0, 0 };
    a.halfExtents = { 0.5f, 0.5f, 0.5f };
    b.shape = xe::ShapeKind::Box;
    b.position = { 0.7f, 0, 0 };  // overlap
    b.halfExtents = { 0.5f, 0.5f, 0.5f };
    a.dynamic = true; b.dynamic = true;
    w.Add(a); w.Add(b);
    int n = w.Step(0.016f);
    EXPECT_GT(n, 0);
}

TEST(ObbTest, TwoCubesApartNoCollision) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b;
    a.shape = xe::ShapeKind::Box;
    a.position = { 0, 0, 0 };
    a.halfExtents = { 0.5f, 0.5f, 0.5f };
    b.shape = xe::ShapeKind::Box;
    b.position = { 5, 0, 0 };
    b.halfExtents = { 0.5f, 0.5f, 0.5f };
    w.Add(a); w.Add(b);
    EXPECT_EQ(w.Step(0.016f), 0);
}

TEST(ObbTest, RotatedCubeDoesntCollideWithSeparatedAxisAligned) {
    // A 1x1x2 box rotated 45 deg about Z — its X-extent is ~1.7.  Place a
    // second 1m cube 1.0m away: still overlapping.  Place it 1.8m away: not.
    xe::PhysicsWorld w;
    xe::RigidBody a;
    a.shape = xe::ShapeKind::Box;
    a.position = { 0, 0, 0 };
    a.halfExtents = { 0.5f, 0.5f, 1.0f };
    a.orientation = xe::Quat::FromAxisAngle({ 0, 0, 1 }, 0.785398f);
    w.Add(a);
    xe::RigidBody b;
    b.shape = xe::ShapeKind::Box;
    b.position = { 1.8f, 0, 0 };
    b.halfExtents = { 0.5f, 0.5f, 0.5f };
    w.Add(b);
    EXPECT_EQ(w.Step(0.016f), 0);
}

// --- OBB vs Sphere ----------------------------------------------------------

TEST(ObbTest, SphereHitsBox) {
    xe::PhysicsWorld w;
    xe::RigidBody box, sph;
    box.shape = xe::ShapeKind::Box;
    box.position = { 0, 0, 0 };
    box.halfExtents = { 0.5f, 0.5f, 0.5f };
    box.dynamic = false;  // wall
    sph.shape = xe::ShapeKind::Sphere;
    sph.position = { 0.7f, 0, 0 };   // sphere center inside box surface (0.5 - 0.3 buffer = 0.2 overlap)
    sph.radius = 0.3f;
    sph.velocity = { -1, 0, 0 };
    w.Add(box); w.Add(sph);
    EXPECT_GT(w.Step(0.016f), 0);
    EXPECT_GT(sph.position.x, 0.5f);  // pushed back from wall
}

TEST(ObbTest, SphereMissesBox) {
    xe::PhysicsWorld w;
    xe::RigidBody box, sph;
    box.shape = xe::ShapeKind::Box;
    box.position = { 0, 0, 0 };
    box.halfExtents = { 0.5f, 0.5f, 0.5f };
    box.dynamic = false;
    sph.shape = xe::ShapeKind::Sphere;
    sph.position = { 5, 0, 0 };
    sph.radius = 0.3f;
    w.Add(box); w.Add(sph);
    EXPECT_EQ(w.Step(0.016f), 0);
}

// --- Rotation / angular impulse --------------------------------------------

TEST(RotationTest, AngularVelocityRotatesOrientation) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.shape = xe::ShapeKind::Box;
    b.halfExtents = { 0.5f, 0.5f, 0.5f };
    b.angVel = { 0, 1.0f, 0 };   // 1 rad/s about Y
    w.Add(b);
    // 60 small steps = ~1s, with damping the actual rotation is ~0.94 rad.
    for (int i = 0; i < 60; ++i) w.Step(1.0f / 60.0f);
    auto worldX = w.Get(0).orientation.Rotate({ 1, 0, 0 });
    // We accept that some damping occurred, so we just check it rotated.
    EXPECT_NEAR(worldX.z, -std::sin(0.94f), 5e-2f);
    EXPECT_NEAR(worldX.x,  std::cos(0.94f), 5e-2f);
    // And the rotation is monotonic — the angle has clearly increased from 0.
    float angle = std::atan2(-worldX.z, worldX.x);
    EXPECT_GT(angle, 0.7f);
    EXPECT_LT(angle, 1.1f);
}

TEST(RotationTest, SpinSetsAngularVelocity) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.shape = xe::ShapeKind::Box;
    b.halfExtents = { 0.5f, 0.5f, 0.5f };
    w.Add(b);
    w.Spin(0, { 0, 2.0f, 0 });
    EXPECT_NEAR(w.Get(0).angVel.y, 2.0f, 1e-5f);
    for (int i = 0; i < 30; ++i) w.Step(1.0f / 60.0f);
    // Rotated X axis should be roughly at -sin(0.94) in Z.
    auto worldX = w.Get(0).orientation.Rotate({ 1, 0, 0 });
    EXPECT_LT(worldX.x, 0.7f);  // rotated away from identity
    EXPECT_LT(worldX.z, -0.5f); // clearly negative Z
}

// --- Capture / Reset --------------------------------------------------------

TEST(ResetTest, ResetRestoresInitialState) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.shape = xe::ShapeKind::Box;
    b.position = { 0, 0, 0 };
    b.halfExtents = { 0.5f, 0.5f, 0.5f };
    w.Add(b);
    w.CaptureInitialState();
    w.Kick(0, 10, 0, 0);
    for (int i = 0; i < 10; ++i) w.Step(0.1f);
    EXPECT_GT(w.Get(0).position.x, 0.0f);
    w.ResetAll();
    EXPECT_NEAR(w.Get(0).position.x, 0.0f, 1e-4f);
    EXPECT_NEAR(w.Get(0).velocity.x, 0.0f, 1e-4f);
}

}  // namespace
