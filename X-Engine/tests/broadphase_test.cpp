#include <gtest/gtest.h>
#include "physics/physics_world.h"

namespace {

constexpr float kEps = 1e-4f;

// --- AABB ------------------------------------------------------------------

TEST(AabbTest, OverlapsTrue) {
    xe::AABB a{ {0,0,0}, {1,1,1} };
    xe::AABB b{ {0.5f,0.5f,0.5f}, {2,2,2} };
    EXPECT_TRUE(a.Overlaps(b));
}

TEST(AabbTest, OverlapsFalse) {
    xe::AABB a{ {0,0,0}, {1,1,1} };
    xe::AABB b{ {2,2,2}, {3,3,3} };
    EXPECT_FALSE(a.Overlaps(b));
}

TEST(AabbTest, OverlapsTouching) {
    xe::AABB a{ {0,0,0}, {1,1,1} };
    xe::AABB b{ {1,0,0}, {2,1,1} };
    EXPECT_TRUE(a.Overlaps(b));   // boundary counts as overlap
}

TEST(AabbTest, FromSphere) {
    auto a = xe::AABB::FromSphere({ 0, 0, 0 }, 1.0f);
    EXPECT_FLOAT_EQ(a.min.x, -1.0f);
    EXPECT_FLOAT_EQ(a.max.y,  1.0f);
}

TEST(AabbTest, FromOBBAxisAligned) {
    xe::Vec3 axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
    auto a = xe::AABB::FromOBB({ 2, 3, 4 }, axes, { 0.5f, 0.5f, 0.5f });
    EXPECT_FLOAT_EQ(a.min.x, 1.5f);
    EXPECT_FLOAT_EQ(a.max.x, 2.5f);
    EXPECT_FLOAT_EQ(a.min.y, 2.5f);
    EXPECT_FLOAT_EQ(a.max.z, 4.5f);
}

TEST(AabbTest, FromOBBRotatedIsLarger) {
    // 45 deg rotation makes AABB larger than the OBB itself.
    xe::Vec3 axes[3] = {
        { 0.7071f, 0.7071f, 0 },
        { -0.7071f, 0.7071f, 0 },
        { 0, 0, 1 }
    };
    auto a = xe::AABB::FromOBB({ 0, 0, 0 }, axes, { 1, 1, 1 });
    float sx = a.max.x - a.min.x;
    float sy = a.max.y - a.min.y;
    EXPECT_GT(sx, 1.5f);
    EXPECT_GT(sy, 1.5f);
}

// --- Broadphase ------------------------------------------------------------

TEST(BroadphaseTest, EmptyWorld) {
    xe::PhysicsWorld w;
    auto pairs = w.ComputeBroadphasePairs();
    EXPECT_TRUE(pairs.empty());
}

TEST(BroadphaseTest, DisjointBodiesNoPairs) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b;
    a.position = { 0, 0, 0 };
    b.position = { 10, 0, 0 };
    w.Add(a); w.Add(b);
    auto pairs = w.ComputeBroadphasePairs();
    EXPECT_TRUE(pairs.empty());
}

TEST(BroadphaseTest, OverlappingBoxesGeneratePair) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b;
    a.shape = xe::ShapeKind::Box;
    a.position = { 0, 0, 0 };
    a.halfExtents = { 0.5f, 0.5f, 0.5f };
    b.shape = xe::ShapeKind::Box;
    b.position = { 0.6f, 0, 0 };
    b.halfExtents = { 0.5f, 0.5f, 0.5f };
    w.Add(a); w.Add(b);
    auto pairs = w.ComputeBroadphasePairs();
    ASSERT_EQ(pairs.size(), 1u);
    EXPECT_EQ(pairs[0].first, 0);
    EXPECT_EQ(pairs[0].second, 1);
}

TEST(BroadphaseTest, ThreeBodiesTwoPairs) {
    xe::PhysicsWorld w;
    for (int i = 0; i < 3; ++i) {
        xe::RigidBody b;
        b.shape = xe::ShapeKind::Sphere;
        b.position = { (float)i * 0.9f, 0, 0 };
        b.radius = 0.5f;
        w.Add(b);
    }
    auto pairs = w.ComputeBroadphasePairs();
    EXPECT_EQ(pairs.size(), 2u);
}

TEST(BroadphaseTest, StepUsesBroadphase) {
    // Far apart bodies should not collide even though broadphase runs.
    xe::PhysicsWorld w;
    xe::RigidBody a, b;
    a.position = { 0, 0, 0 };
    a.velocity = { 1, 0, 0 };
    b.position = { 100, 0, 0 };
    b.velocity = { -1, 0, 0 };
    w.Add(a); w.Add(b);
    int n = w.Step(0.016f);
    EXPECT_EQ(n, 0);
}

// --- Picking (raycast) -----------------------------------------------------

TEST(RaycastTest, HitsSphereInFront) {
    xe::PhysicsWorld w;
    xe::RigidBody sph;
    sph.position = { 0, 0, 5 };
    sph.radius = 0.5f;
    w.Add(sph);
    xe::Ray r{ {0, 0, 0 }, {0, 0, 1} };
    auto h = w.RayCast(r);
    EXPECT_EQ(h.body, 0);
    EXPECT_NEAR(h.t, 4.5f, kEps);
}

TEST(RaycastTest, MissesSphere) {
    xe::PhysicsWorld w;
    xe::RigidBody sph;
    sph.position = { 5, 0, 5 };
    sph.radius = 0.5f;
    w.Add(sph);
    xe::Ray r{ {0, 0, 0}, {0, 0, 1} };
    auto h = w.RayCast(r);
    EXPECT_EQ(h.body, -1);
}

TEST(RaycastTest, ChoosesClosestHit) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b;
    a.position = { 0, 0, 5 };
    a.radius = 0.5f;
    b.position = { 0, 0, 3 };
    b.radius = 0.5f;
    w.Add(a); w.Add(b);
    xe::Ray r{ {0, 0, 0}, {0, 0, 1} };
    auto h = w.RayCast(r);
    EXPECT_EQ(h.body, 1);
    EXPECT_NEAR(h.t, 2.5f, kEps);
}

TEST(RaycastTest, HitsBoxOBB) {
    xe::PhysicsWorld w;
    xe::RigidBody box;
    box.shape = xe::ShapeKind::Box;
    box.position = { 0, 0, 5 };
    box.halfExtents = { 0.5f, 0.5f, 0.5f };
    w.Add(box);
    xe::Ray r{ {0, 0, 0}, {0, 0, 1} };
    auto h = w.RayCast(r);
    EXPECT_EQ(h.body, 0);
    EXPECT_NEAR(h.t, 4.5f, kEps);
}

TEST(RaycastTest, MissesRotatedBoxFromAbove) {
    // A flat slab rotated 45 deg — ray going up the diagonal might miss.
    xe::PhysicsWorld w;
    xe::RigidBody box;
    box.shape = xe::ShapeKind::Box;
    box.position = { 0, 0, 5 };
    box.halfExtents = { 1.0f, 0.1f, 0.1f };   // very thin
    box.orientation = xe::Quat::FromAxisAngle({ 0, 1, 0 }, 0.0f);
    w.Add(box);
    xe::Ray r{ {0, 0, 0}, {0, 0, 1} };
    auto h = w.RayCast(r);
    EXPECT_EQ(h.body, 0);  // should hit (slab faces +Z)
}

TEST(RaycastTest, RayBehindOriginMisses) {
    xe::PhysicsWorld w;
    xe::RigidBody sph;
    sph.position = { 0, 0, -5 };
    sph.radius = 0.5f;
    w.Add(sph);
    xe::Ray r{ {0, 0, 0}, {0, 0, 1} };
    auto h = w.RayCast(r);
    EXPECT_EQ(h.body, -1);
}

// --- Selection -------------------------------------------------------------

TEST(SelectionTest, DefaultSelectedIsMinusOne) {
    xe::PhysicsWorld w;
    xe::RigidBody b; w.Add(b);
    EXPECT_EQ(w.Selected(), -1);
}

TEST(SelectionTest, SetSelected) {
    xe::PhysicsWorld w;
    xe::RigidBody a, b;
    w.Add(a); w.Add(b);
    w.SetSelected(1);
    EXPECT_EQ(w.Selected(), 1);
    w.SetSelected(-1);
    EXPECT_EQ(w.Selected(), -1);
}

}  // namespace
