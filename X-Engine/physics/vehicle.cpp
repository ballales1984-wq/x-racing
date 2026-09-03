#include "physics/vehicle.h"
#include <cmath>

namespace xe {

void Vehicle::Build(PhysicsWorld& w, Vec3 position) {
    // Chassis: 2.0 x 0.5 x 1.2 box.
    Vec3 chassisHalf = { 1.0f, 0.25f, 0.6f };
    chassisIdx = w.AddBox(position, chassisHalf, chassisMass);

    // Local-space anchor positions on the chassis for each wheel.
    float hx = wheelBase   * 0.5f;
    float hz = trackWidth  * 0.5f;
    float yDown = -chassisHalf.y - suspensionRest;  // below chassis center
    Vec3 localAnchors[4] = {
        { +hx, yDown, -hz },  // FL
        { +hx, yDown, +hz },  // FR
        { -hx, yDown, -hz },  // RL
        { -hx, yDown, +hz },  // RR
    };

    for (int i = 0; i < 4; ++i) {
        // World position of the wheel: chassis pos + rotated local anchor.
        Vec3 worldPos = {
            position.x + localAnchors[i].x,
            position.y + localAnchors[i].y,
            position.z + localAnchors[i].z,
        };
        wheelIdx[i] = w.AddSphere(worldPos, wheelRadius, wheelMass);
        // Suspension: distance constraint at the rest length.
        DistanceConstraint c;
        c.a = chassisIdx;
        c.b = wheelIdx[i];
        c.restLength = suspensionRest;
        c.stiffness = 1.0f;
        suspensionIdx[i] = w.AddConstraint(c);
    }
}

void Vehicle::Update(PhysicsWorld& w, float dt) {
    if (chassisIdx < 0) return;

    // Steer angle ramps toward target.
    float target = steer * maxSteer;
    float maxDelta = maxSteerRate * dt;
    if (steerAngle < target) steerAngle = std::min(steerAngle + maxDelta, target);
    else                    steerAngle = std::max(steerAngle - maxDelta, target);

    // Chassis world axes.
    RigidBody& chassis = w.Get(chassisIdx);
    Vec3 fwdLocal = { 1, 0, 0 };  // chassis +X is forward
    Vec3 rightLocal = { 0, 0, 1 };
    Vec3 upLocal = { 0, 1, 0 };
    Vec3 fwd   = chassis.orientation.Rotate(fwdLocal);
    Vec3 right = chassis.orientation.Rotate(rightLocal);
    Vec3 up    = chassis.orientation.Rotate(upLocal);

    // Per-wheel force application.
    for (int i = 0; i < 4; ++i) {
        RigidBody& w0 = w.Get(wheelIdx[i]);
        if (!w0.dynamic) continue;

        // 1) Longitudinal force (drive / brake along forward).
        float vForward = Dot(w0.velocity, fwd);
        float drive = 0.0f;
        bool driven = (i == RL || i == RR);  // RWD
        if (driven) {
            drive = throttle * engineForce;
        }
        if (brake > 0.0f) {
            // Apply brake as a force opposing forward velocity.
            float sign = (vForward > 0.01f) ? -1.0f : (vForward < -0.01f ? 1.0f : 0.0f);
            drive += sign * brake * brakeForce;
        }
        // Rolling friction: always opposes forward velocity.
        drive -= vForward * rollingFriction * w0.mass;
        w0.velocity.x += (fwd.x * drive / w0.mass) * dt;
        w0.velocity.y += (fwd.y * drive / w0.mass) * dt;
        w0.velocity.z += (fwd.z * drive / w0.mass) * dt;

        // 2) Lateral grip: kill sideways velocity.  Front wheels have a
        // steered forward direction (fwd rotated by steerAngle around up).
        Vec3 wheelFwd = fwd;
        if (i == FL || i == FR) {
            float cs = std::cos(steerAngle);
            float sn = std::sin(steerAngle);
            wheelFwd = { fwd.x * cs + right.x * sn,
                         fwd.y * cs + right.y * sn,
                         fwd.z * cs + right.z * sn };
        }
        // Project velocity onto wheel forward.
        float vF = Dot(w0.velocity, wheelFwd);
        // Lateral axis is the component perpendicular to wheelFwd in the
        // horizontal plane.
        Vec3 lateral = w0.velocity - wheelFwd * vF;
        // Apply damping to lateral velocity.
        float damp = std::min(1.0f, lateralGrip * dt);
        w0.velocity = wheelFwd * vF + lateral * (1.0f - damp);

        (void)up; (void)right;  // reserved for future
    }
}

}  // namespace xe
