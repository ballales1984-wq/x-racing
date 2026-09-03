#pragma once

#include "physics/physics_world.h"

namespace xe {

// Simple vehicle model.  One chassis (box) + 4 wheels (spheres) tied to the
// chassis with distance constraints that act as suspension springs.
//
// The chassis is the dynamic body the player drives.  The wheels are dynamic
// too but stay attached via constraints; Update() applies drive / brake /
// steer forces per wheel.
struct Vehicle {
    // Configuration (set before Build).
    float wheelBase       = 2.4f;   // distance front-rear axle
    float trackWidth      = 1.6f;   // distance left-right wheels on same axle
    float suspensionRest  = 0.6f;   // rest length of suspension constraint
    float wheelRadius     = 0.35f;
    float chassisMass     = 800.0f;
    float wheelMass       = 30.0f;
    float maxSteer        = 0.6f;   // rad (~34 deg)
    float maxSteerRate    = 4.0f;   // rad/sec
    float engineForce     = 4000.0f; // N per wheel
    float brakeForce      = 6000.0f;
    float rollingFriction = 0.4f;   // longitudinal friction coeff
    float lateralGrip     = 12.0f;  // lateral grip coefficient

    // Built by Build().
    int chassisIdx = -1;
    int wheelIdx[4] = { -1, -1, -1, -1 };
    int suspensionIdx[4] = { -1, -1, -1, -1 };
    float steerAngle = 0.0f;        // current steering angle (front wheels)

    // Driver inputs (set by main loop each frame).
    float throttle = 0.0f;          // -1..+1
    float brake    = 0.0f;          // 0..1
    float steer    = 0.0f;          // -1..+1

    // Wheel index helpers: 0=FL, 1=FR, 2=RL, 3=RR.
    static constexpr int FL = 0;
    static constexpr int FR = 1;
    static constexpr int RL = 2;
    static constexpr int RR = 3;

    // Build the vehicle in the given world at the given position.
    void Build(PhysicsWorld& w, Vec3 position);

    // Apply driver inputs and per-wheel forces. Call before Step().
    void Update(PhysicsWorld& w, float dt);
};

}  // namespace xe
