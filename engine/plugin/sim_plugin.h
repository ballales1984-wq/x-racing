#pragma once

// Project 0 — C plugin interface for external simulators
// Provides a C ABI so external tools (Unity, etc.) can drive the simulation.

#ifdef __cplusplus
extern "C" {
#endif

    // Plain-old-data snapshot of the vehicle for external consumers.
    typedef struct {
        double x;
        double y;
        double vx;
        double vy;
        double heading;
        double speed;
        double rpm;
        int gear;
        double throttle;
        double brake;
        double steer;
    } VehicleState;

    // Initialize the simulation with a default track and reset the car.
    // Returns 0 on success; safe to call multiple times (no-op after first).
    __declspec(dllexport) int SimPlugin_Initialize();

    // Advance the simulation by one step using the provided driver controls.
    // dt is currently unused; the simulation uses its own fixed timestep.
    __declspec(dllexport) void SimPlugin_Update(double dt, double throttle, double brake, double steer);

    // Copy the current vehicle state into the caller-provided struct.
    __declspec(dllexport) void SimPlugin_GetVehicleState(VehicleState* state);

#ifdef __cplusplus
}
#endif
