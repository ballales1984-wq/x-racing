#include <gtest/gtest.h>
#include "physics/physics_world.h"
#include <cstdio>

namespace {

constexpr float kEps = 1e-3f;

TEST(SerializeTest, EmptyRoundTrip) {
    xe::PhysicsWorld w;
    auto text = w.Serialize();
    EXPECT_FALSE(text.empty());
    EXPECT_TRUE(w.Deserialize(text));
    EXPECT_EQ(w.Size(), 0);
}

TEST(SerializeTest, BodiesRoundTrip) {
    xe::PhysicsWorld w;
    w.AddSphere({ 1, 2, 3 }, 0.5f, 1.5f);
    w.AddBox({ 0, 1, 0 }, { 0.5f, 0.5f, 0.5f }, 2.0f);
    w.AddStaticBox({ 0, -2, 0 }, { 5, 0.25f, 5 });
    auto text = w.Serialize();
    xe::PhysicsWorld w2;
    EXPECT_TRUE(w2.Deserialize(text));
    EXPECT_EQ(w2.Size(), 3);
    EXPECT_NEAR(w2.Get(0).position.x, 1.0f, kEps);
    EXPECT_NEAR(w2.Get(1).position.y, 1.0f, kEps);
    EXPECT_NEAR(w2.Get(2).position.y, -2.0f, kEps);
    EXPECT_FALSE(w2.Get(2).dynamic);
}

TEST(SerializeTest, ConstraintsRoundTrip) {
    xe::PhysicsWorld w;
    w.AddSphere({ 0, 0, 0 }, 0.5f);
    w.AddSphere({ 1, 0, 0 }, 0.5f);
    w.AddSphere({ 2, 0, 0 }, 0.5f);
    xe::DistanceConstraint c; c.a = 0; c.b = 1; c.restLength = 1.0f;
    w.AddConstraint(c);
    xe::DistanceConstraint c2; c2.a = 1; c2.b = 2; c2.restLength = 1.0f;
    w.AddConstraint(c2);
    auto text = w.Serialize();
    xe::PhysicsWorld w2;
    EXPECT_TRUE(w2.Deserialize(text));
    EXPECT_EQ(w2.NumConstraints(), 2);
}

TEST(SerializeTest, JointsRoundTrip) {
    xe::PhysicsWorld w;
    w.AddBox({ 0, 0, 0 }, { 0.5f, 0.5f, 0.5f });
    w.AddBox({ 2, 0, 0 }, { 0.5f, 0.5f, 0.5f });
    xe::BallJoint bj; bj.a = 0; bj.b = 1; bj.localA = { 0.5f, 0, 0 };
    w.AddBallJoint(bj);
    xe::HingeJoint hj; hj.a = 0; hj.b = 1; hj.localAxisA = { 0, 1, 0 };
    w.AddHingeJoint(hj);
    auto text = w.Serialize();
    xe::PhysicsWorld w2;
    EXPECT_TRUE(w2.Deserialize(text));
    EXPECT_EQ(w2.NumBallJoints(), 1);
    EXPECT_EQ(w2.NumHingeJoints(), 1);
    EXPECT_NEAR(w2.GetBallJoint(0).localA.x, 0.5f, kEps);
    EXPECT_NEAR(w2.GetHingeJoint(0).localAxisA.y, 1.0f, kEps);
}

TEST(SerializeTest, GravityRoundTrip) {
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    w.SetGravityEnabled(true);
    auto text = w.Serialize();
    xe::PhysicsWorld w2;
    EXPECT_TRUE(w2.Deserialize(text));
    EXPECT_TRUE(w2.GravityEnabled());
    EXPECT_NEAR(w2.Gravity().y, -9.81f, kEps);
}

TEST(SerializeTest, OrientationRoundTrip) {
    xe::PhysicsWorld w;
    xe::RigidBody b;
    b.shape = xe::ShapeKind::Box;
    b.orientation = xe::Quat::FromAxisAngle({ 0, 1, 0 }, 1.234f);
    w.Add(b);
    auto text = w.Serialize();
    xe::PhysicsWorld w2;
    EXPECT_TRUE(w2.Deserialize(text));
    auto worldX = w2.Get(0).orientation.Rotate({ 1, 0, 0 });
    EXPECT_NEAR(worldX.x, std::cos(1.234f), 1e-3f);
}

TEST(SerializeTest, SaveLoadFile) {
    xe::PhysicsWorld w;
    w.AddSphere({ 0, 0, 0 }, 0.5f);
    w.AddSphere({ 1, 0, 0 }, 0.5f);
    xe::DistanceConstraint c; c.a = 0; c.b = 1; c.restLength = 1.0f;
    w.AddConstraint(c);
    const char* path = "test_serialize.scn";
    EXPECT_TRUE(w.SaveToFile(path));
    xe::PhysicsWorld w2;
    EXPECT_TRUE(w2.LoadFromFile(path));
    EXPECT_EQ(w2.Size(), 2);
    EXPECT_EQ(w2.NumConstraints(), 1);
    std::remove(path);
}

TEST(SerializeTest, LoadFileMissingFails) {
    xe::PhysicsWorld w;
    EXPECT_FALSE(w.LoadFromFile("/nonexistent/path/foo.scn"));
}

TEST(SerializeTest, BadTextFails) {
    xe::PhysicsWorld w;
    EXPECT_FALSE(w.Deserialize("not a scene"));
}

// --- SpawnAt ---------------------------------------------------------------

TEST(SpawnTest, SpawnSphereAtAdds) {
    xe::PhysicsWorld w;
    int idx = w.SpawnSphereAt({ 1, 2, 3 }, 0.5f, 1.0f);
    EXPECT_EQ(idx, 0);
    EXPECT_FLOAT_EQ(w.Get(0).position.x, 1.0f);
}

}  // namespace
