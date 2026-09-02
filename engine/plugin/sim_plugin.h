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

    // Geographic + track-relative snapshot produced by the tracking layer.
    // Combines the simulated GPS sample (latitude/longitude/altitude) with the
    // mapped track position (S / L). Returns 0 on success, -1 if unavailable.
    typedef struct {
        double latitude;     // deg, WGS84 latitude
        double longitude;    // deg, WGS84 longitude
        double altitude;     // m, altitude
        double track_s;      // m, distance along centerline
        double track_l;      // m, lateral offset from centerline
        double speed;        // m/s, ground speed
        double heading;      // rad, true heading
        double accuracy;     // m, 1-sigma horizontal accuracy
        int on_track;        // 1 if mapped within track boundaries
    } TrackingSample;

    __declspec(dllexport) int SimPlugin_GetTracking(TrackingSample* out);

    // Release all resources held by the plugin. Safe to call multiple times.
    // After this call, SimPlugin_Initialize must be called again before use.
    __declspec(dllexport) void SimPlugin_Shutdown();

#ifdef __cplusplus
}
#endif
