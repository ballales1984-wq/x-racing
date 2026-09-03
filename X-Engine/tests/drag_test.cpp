#include <gtest/gtest.h>
#include "physics/physics_world.h"

namespace {

constexpr float kEps = 1e-3f;

TEST(DragTest, NotDraggingByDefault) {
    xe::PhysicsWorld w;
    xe::RigidBody b; w.Add(b);
    EXPECT_FALSE(w.IsDragging());
    EXPECT_EQ(w.DragIndex(), -1);
}

TEST(DragTest, BeginDragOnValidBody) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.position = { 0, 0, 5 };
    w.Add(b);
    EXPECT_TRUE(w.BeginDrag(0, { 0.2f, 0.1f, 5.0f }));
    EXPECT_TRUE(w.IsDragging());
    EXPECT_EQ(w.DragIndex(), 0);
}

TEST(DragTest, BeginDragFailsForStatic) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.dynamic = false;
    w.Add(b);
    EXPECT_FALSE(w.BeginDrag(0, { 0, 0, 0 }));
    EXPECT_FALSE(w.IsDragging());
}

TEST(DragTest, BeginDragFailsForInvalidIndex) {
    xe::PhysicsWorld w;
    EXPECT_FALSE(w.BeginDrag(5, { 0, 0, 0 }));
    EXPECT_FALSE(w.BeginDrag(-1, { 0, 0, 0 }));
}

TEST(DragTest, EndDragClearsState) {
    xe::PhysicsWorld w;
    xe::RigidBody b; w.Add(b);
    w.BeginDrag(0, { 0, 0, 0 });
    w.EndDrag();
    EXPECT_FALSE(w.IsDragging());
}

TEST(DragTest, DeselectEndsDrag) {
    xe::PhysicsWorld w;
    xe::RigidBody b; w.Add(b);
    w.BeginDrag(0, { 0, 0, 0 });
    w.SetSelected(-1);
    EXPECT_FALSE(w.IsDragging());
}

TEST(DragTest, UpdateDragAnchorChangesAnchor) {
    xe::PhysicsWorld w;
    xe::RigidBody b; b.position = { 0, 0, 0 }; w.Add(b);
    w.BeginDrag(0, { 0, 0, 0 });
    w.UpdateDragAnchor({ 1, 0, 0 });
    EXPECT_NEAR(w.DragAnchor().x, 1.0f, kEps);
}

TEST(DragTest, DragMovesBody) {
    // Anchor moves +X by 1 over 1s -> body should follow.
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.position = { 0, 0, 0 };
    b.mass = 1.0f;
    w.Add(b);
    w.BeginDrag(0, { 0, 0, 0 });
    // Simulate 60 frames where the anchor moves +1/60 per frame.
    for (int i = 0; i < 60; ++i) {
        w.UpdateDragAnchor({ (i + 1) / 60.0f, 0, 0 });
        w.Step(1.0f / 60.0f);
    }
    // Body's anchor should be near the last anchor (1, 0, 0).
    auto& body = w.Get(0);
    EXPECT_NEAR(body.position.x, 1.0f, 5e-2f);
}

TEST(DragTest, DragKillsAngularVelocity) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.angVel = { 1, 0, 0 };
    w.Add(b);
    w.BeginDrag(0, { 0, 0, 0 });
    w.UpdateDragAnchor({ 0.1f, 0, 0 });
    w.Step(0.016f);
    EXPECT_NEAR(w.Get(0).angVel.x, 0.0f, kEps);
}

TEST(DragTest, StaticBodyCannotBeDragged) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.dynamic = false;
    w.Add(b);
    w.BeginDrag(0, { 0, 0, 0 });
    EXPECT_FALSE(w.IsDragging());
}

}  // namespace
