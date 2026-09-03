#include <gtest/gtest.h>
#include "physics/physics_world.h"
#include <cmath>

namespace {

constexpr float kEps = 1e-3f;

TEST(SpawnTest, AddSphereReturnsIndex) {
    xe::PhysicsWorld w;
    int idx = w.AddSphere({ 0, 0, 0 }, 0.5f, 1.0f);
    EXPECT_EQ(idx, 0);
    EXPECT_EQ(w.Size(), 1);
    EXPECT_FLOAT_EQ(w.Get(0).radius, 0.5f);
    EXPECT_FLOAT_EQ(w.Get(0).position.x, 0.0f);
    EXPECT_TRUE(w.Get(0).dynamic);
}

TEST(SpawnTest, AddBoxReturnsIndex) {
    xe::PhysicsWorld w;
    int idx = w.AddBox({ 1, 2, 3 }, { 0.5f, 0.5f, 0.5f }, 2.0f);
    EXPECT_EQ(idx, 0);
    auto& b = w.Get(0);
    EXPECT_FLOAT_EQ(b.halfExtents.x, 0.5f);
    EXPECT_FLOAT_EQ(b.mass, 2.0f);
    EXPECT_EQ(b.shape, xe::ShapeKind::Box);
}

TEST(SpawnTest, AddStaticBoxIsStatic) {
    xe::PhysicsWorld w;
    int idx = w.AddStaticBox({ 0, 0, 0 }, { 5, 0.5f, 5 });
    EXPECT_FALSE(w.Get(idx).dynamic);
}

TEST(SpawnTest, MultipleSpawnsIncrement) {
    xe::PhysicsWorld w;
    int a = w.AddSphere({ 0, 0, 0 }, 0.5f);
    int b = w.AddBox({ 0, 1, 0 }, { 0.5f, 0.5f, 0.5f });
    int c = w.AddSphere({ 0, 2, 0 }, 0.3f);
    EXPECT_EQ(a, 0);
    EXPECT_EQ(b, 1);
    EXPECT_EQ(c, 2);
    EXPECT_EQ(w.Size(), 3);
}

// --- Rope ------------------------------------------------------------------

TEST(RopeTest, BuildFreeRope) {
    xe::PhysicsWorld w;
    int first = w.BuildRope(-1, { 0, 5, 0 }, 5, 0.3f, 0.1f, 0.2f);
    EXPECT_GE(first, 0);
    EXPECT_EQ(w.Size(), 5);
}

TEST(RopeTest, BuildRopeWithAnchor) {
    // First bead is connected to the anchor body via distance constraint.
    xe::PhysicsWorld w;
    int anchor = w.AddStaticBox({ 0, 5, 0 }, { 0.1f, 0.1f, 0.1f });
    int first = w.BuildRope(anchor, { 0, 5, 0 }, 4, 0.3f, 0.1f, 0.2f);
    EXPECT_GE(first, 0);
    EXPECT_EQ(w.NumConstraints(), 4);  // 4 segments
}

TEST(RopeTest, RopeFallsUnderGravity) {
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    w.SetGravityEnabled(true);
    int first = w.BuildRope(-1, { 0, 5, 0 }, 5, 0.3f, 0.1f, 0.2f);
    auto& head = w.Get(first);
    auto& tail = w.Get(first + 4);
    float headStartY = head.position.y;
    float tailStartY = tail.position.y;
    for (int i = 0; i < 60; ++i) w.Step(1.0f / 60.0f);
    // Both head and tail should have moved downward.
    EXPECT_LT(w.Get(first).position.y, headStartY);
    EXPECT_LT(w.Get(first + 4).position.y, tailStartY);
}

TEST(RopeTest, RopeSegmentsMaintainLength) {
    // After settling, each segment should be approximately rest length.
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    w.SetGravityEnabled(true);
    float segLen = 0.4f;
    int first = w.BuildRope(-1, { 0, 5, 0 }, 6, segLen, 0.1f, 0.2f);
    for (int i = 0; i < 240; ++i) w.Step(1.0f / 60.0f);
    for (int i = 0; i < 5; ++i) {
        auto& a = w.Get(first + i);
        auto& b = w.Get(first + i + 1);
        float d = std::sqrt((a.position.x - b.position.x) * (a.position.x - b.position.x) +
                             (a.position.y - b.position.y) * (a.position.y - b.position.y) +
                             (a.position.z - b.position.z) * (a.position.z - b.position.z));
        EXPECT_NEAR(d, segLen, 0.05f);
    }
}

}  // namespace
