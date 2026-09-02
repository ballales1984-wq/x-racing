#include <gtest/gtest.h>
#include "physics/physics_world.h"
#include <cmath>

TEST(PhysicsTest, EmptyWorld) {
    xe::PhysicsWorld w;
    EXPECT_EQ(w.Size(), 0);
    EXPECT_EQ(w.Step(0.016f), 0);
}

TEST(PhysicsTest, AddAndGet) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.position = { 1.0f, 2.0f, 3.0f };
    b.radius = 0.5f;
    b.mass = 2.0f;
    int idx = w.Add(b);
    EXPECT_EQ(idx, 0);
    EXPECT_EQ(w.Size(), 1);
    EXPECT_FLOAT_EQ(w.Get(0).position.x, 1.0f);
    EXPECT_FLOAT_EQ(w.Get(0).mass, 2.0f);
}

TEST(PhysicsTest, DisabledDoesNothing) {
    xe::PhysicsWorld w;
    w.SetEnabled(false);
    xe::RigidBody b;
    b.position = { 0, 0, 0 };
    b.velocity = { 1, 0, 0 };
    w.Add(b);
    w.Step(1.0f);
    EXPECT_FLOAT_EQ(w.Get(0).position.x, 0.0f);  // unchanged
}

TEST(PhysicsTest, IntegrationAdvancesPosition) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.position = { 0, 0, 0 };
    b.velocity = { 1, 0, 0 };
    b.dynamic = true;
    w.Add(b);
    w.Step(2.0f, 1.0f);  // no damping
    EXPECT_FLOAT_EQ(w.Get(0).position.x, 2.0f);
}

TEST(PhysicsTest, DampingReducesVelocity) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.velocity = { 10, 0, 0 };
    w.Add(b);
    w.Step(1.0f, 0.5f);
    EXPECT_FLOAT_EQ(w.Get(0).velocity.x, 5.0f);
}

TEST(PhysicsTest, TwoBodiesCollide) {
    xe::PhysicsWorld w;
    xe::RigidBody a;
    a.position = { 0, 0, 0 };
    a.radius = 0.5f;
    a.mass = 1.0f;
    a.velocity = { 1, 0, 0 };
    xe::RigidBody b;
    b.position = { 0.8f, 0, 0 };
    b.radius = 0.5f;
    b.mass = 1.0f;
    b.velocity = { 0, 0, 0 };
    w.Add(a);
    w.Add(b);
    int n = w.Step(0.016f);
    EXPECT_GT(n, 0);
    EXPECT_LT(w.Get(0).velocity.x, 1.0f);  // bounced/slowed
}

TEST(PhysicsTest, NoCollisionWhenApart) {
    xe::PhysicsWorld w;
    xe::RigidBody a; a.position = { 0, 0, 0 }; a.radius = 0.5f;
    xe::RigidBody b; b.position = { 5, 0, 0 }; b.radius = 0.5f;
    w.Add(a);
    w.Add(b);
    int n = w.Step(0.016f);
    EXPECT_EQ(n, 0);
}

TEST(PhysicsTest, StaticBodyNotMoved) {
    xe::PhysicsWorld w;
    xe::RigidBody a; a.position = { 0, 0, 0 }; a.radius = 0.5f;
    a.velocity = { 1, 0, 0 }; a.dynamic = true;
    xe::RigidBody b; b.position = { 0.8f, 0, 0 }; b.radius = 0.5f;
    b.dynamic = false;  // wall
    w.Add(a);
    w.Add(b);
    w.Step(0.016f);
    EXPECT_FLOAT_EQ(w.Get(1).position.x, 0.8f);  // static
}

TEST(PhysicsTest, KickAppliesImpulse) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.mass = 2.0f;
    w.Add(b);
    w.Kick(0, 4.0f, 0, 0);
    EXPECT_FLOAT_EQ(w.Get(0).velocity.x, 2.0f);  // 4 / 2 mass
}

TEST(PhysicsTest, KickIgnoredForStatic) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.dynamic = false;
    b.mass = 1.0f;
    w.Add(b);
    w.Kick(0, 5.0f, 0, 0);
    EXPECT_FLOAT_EQ(w.Get(0).velocity.x, 0.0f);
}

TEST(PhysicsTest, Clear) {
    xe::PhysicsWorld w;
    xe::RigidBody b; w.Add(b);
    xe::RigidBody b2; w.Add(b2);
    EXPECT_EQ(w.Size(), 2);
    w.Clear();
    EXPECT_EQ(w.Size(), 0);
}

TEST(PhysicsTest, CollisionsRecorded) {
    xe::PhysicsWorld w;
    xe::RigidBody a; a.position = { 0.0f, 0.0f, 0.0f }; a.radius = 0.5f;
    xe::RigidBody b; b.position = { 0.7f, 0.0f, 0.0f }; b.radius = 0.5f;
    w.Add(a); w.Add(b);
    w.Step(0.016f);
    ASSERT_EQ(w.LastCollisionCount(), 1);
    EXPECT_FLOAT_EQ(static_cast<float>(w.LastCollisions()[0].a), 0.0f);
    EXPECT_FLOAT_EQ(static_cast<float>(w.LastCollisions()[0].b), 1.0f);
}