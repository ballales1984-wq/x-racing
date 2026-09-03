#include <gtest/gtest.h>
#include "physics/vehicle.h"
#include "physics/physics_world.h"
#include <cmath>

namespace {

constexpr float kEps = 1e-3f;

TEST(VehicleTest, BuildCreates5Bodies) {
    xe::PhysicsWorld w;
    xe::Vehicle v;
    v.Build(w, { 0, 1.0f, 0 });
    EXPECT_EQ(w.Size(), 5);
    EXPECT_GE(v.chassisIdx, 0);
    for (int i = 0; i < 4; ++i) EXPECT_GE(v.wheelIdx[i], 0);
}

TEST(VehicleTest, BuildCreates4Constraints) {
    xe::PhysicsWorld w;
    xe::Vehicle v;
    v.Build(w, { 0, 1.0f, 0 });
    EXPECT_EQ(w.NumConstraints(), 4);
}

TEST(VehicleTest, ChassisHoldsWheelsAboveGround) {
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    w.SetGravityEnabled(true);
    xe::Vehicle v;
    v.Build(w, { 0, 2.0f, 0 });
    xe::RigidBody ground;
    ground.shape = xe::ShapeKind::Box;
    ground.position = { 0, -1.0f, 0 };
    ground.halfExtents = { 10, 0.25f, 10 };
    ground.dynamic = false;
    w.Add(ground);
    for (int i = 0; i < 240; ++i) w.Step(1.0f / 60.0f);
    // Chassis should be supported above the ground by the wheels.
    EXPECT_GT(w.Get(v.chassisIdx).position.y, -0.3f);
    // At least one wheel should be near ground level.
    int nearGround = 0;
    for (int i = 0; i < 4; ++i) {
        if (w.Get(v.wheelIdx[i]).position.y < 0.0f) ++nearGround;
    }
    EXPECT_GE(nearGround, 1);
}

TEST(VehicleTest, ThrottleMovesForward) {
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    w.SetGravityEnabled(true);
    xe::Vehicle v;
    v.Build(w, { 0, 2.0f, 0 });
    xe::RigidBody ground;
    ground.shape = xe::ShapeKind::Box;
    ground.position = { 0, -1.0f, 0 };
    ground.halfExtents = { 10, 0.25f, 10 };
    ground.dynamic = false;
    w.Add(ground);
    for (int i = 0; i < 60; ++i) w.Step(1.0f / 60.0f);
    float startX = w.Get(v.chassisIdx).position.x;
    v.throttle = 1.0f;
    for (int i = 0; i < 120; ++i) {
        v.Update(w, 1.0f / 60.0f);
        w.Step(1.0f / 60.0f);
    }
    float moved = w.Get(v.chassisIdx).position.x - startX;
    EXPECT_GT(moved, 0.5f);
}

TEST(VehicleTest, SteerChangesDirection) {
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    w.SetGravityEnabled(true);
    xe::Vehicle v;
    v.Build(w, { 0, 2.0f, 0 });
    xe::RigidBody ground;
    ground.shape = xe::ShapeKind::Box;
    ground.position = { 0, -1.0f, 0 };
    ground.halfExtents = { 10, 0.25f, 10 };
    ground.dynamic = false;
    w.Add(ground);
    for (int i = 0; i < 60; ++i) w.Step(1.0f / 60.0f);
    v.throttle = 1.0f;
    v.steer    = 1.0f;
    for (int i = 0; i < 120; ++i) {
        v.Update(w, 1.0f / 60.0f);
        w.Step(1.0f / 60.0f);
    }
    // Right steer pushes chassis to -Z (right-handed, +Y up).
    EXPECT_LT(w.Get(v.chassisIdx).position.z, -0.2f);
}

TEST(VehicleTest, SteerRampsToTarget) {
    xe::PhysicsWorld w;
    xe::Vehicle v;
    v.Build(w, { 0, 2.0f, 0 });
    v.steer = 1.0f;
    // 0.1s of steering: angle = 4*0.1 = 0.4 rad, less than maxSteer (0.6).
    for (int i = 0; i < 2; ++i) v.Update(w, 0.05f);
    EXPECT_LT(v.steerAngle, v.maxSteer);
    EXPECT_GT(v.steerAngle, 0.0f);
    // Long enough: full steer.
    v.steerAngle = 0;
    v.steer = 1.0f;
    for (int i = 0; i < 100; ++i) v.Update(w, 0.05f);
    EXPECT_NEAR(v.steerAngle, v.maxSteer, 0.01f);
}

TEST(VehicleTest, BrakeDecelerates) {
    xe::PhysicsWorld w;
    w.SetGravity({ 0, -9.81f, 0 });
    w.SetGravityEnabled(true);
    xe::Vehicle v;
    v.Build(w, { 0, 2.0f, 0 });
    xe::RigidBody ground;
    ground.shape = xe::ShapeKind::Box;
    ground.position = { 0, -1.0f, 0 };
    ground.halfExtents = { 10, 0.25f, 10 };
    ground.dynamic = false;
    w.Add(ground);
    for (int i = 0; i < 60; ++i) w.Step(1.0f / 60.0f);
    v.throttle = 1.0f;
    for (int i = 0; i < 120; ++i) {
        v.Update(w, 1.0f / 60.0f);
        w.Step(1.0f / 60.0f);
    }
    v.throttle = 0.0f;
    v.brake = 1.0f;
    for (int i = 0; i < 60; ++i) {
        v.Update(w, 1.0f / 60.0f);
        w.Step(1.0f / 60.0f);
    }
    const auto& w0 = w.Get(v.wheelIdx[xe::Vehicle::RL]);
    float speed = std::sqrt(w0.velocity.x*w0.velocity.x +
                            w0.velocity.y*w0.velocity.y +
                            w0.velocity.z*w0.velocity.z);
    EXPECT_LT(speed, 5.0f);
}

}  // namespace
