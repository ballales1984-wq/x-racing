#include "plugin/sim_plugin.h"
#include "simulation/simulation.h"
#include "track/track.h"
#include "vehicle/vehicle.h"
#include "input/input.h"

// Global simulation instance used by the C plugin ABI.
// This singleton is shared across all DLL calls and persists for the
// lifetime of the loaded module.
// NOTE: Simulation stores a pointer to the track, so the track must outlive
// the simulation; keep it as a persistent static rather than a local.
static p0::track::Track g_track;
static p0::simulation::Simulation g_sim;
static bool g_initialized = false;

extern "C" {

    // Initialize the simulation: build a default track and reset the car
    // to the start line. Safe to call once; subsequent calls are ignored.
    // Returns 0 on success.
    __declspec(dllexport) int SimPlugin_Initialize() {
        if (g_initialized) return 0;

        g_track = p0::track::Track();
        g_sim.set_track(g_track);

        p0::vehicle::VehicleState initial;
        initial.position = g_track.get_start_position();
        initial.heading = g_track.get_start_heading();
        initial.speed = 0.0;

        g_sim.reset(initial);
        g_initialized = true;
        return 0;
    }

    // Advance the simulation by one step using the provided driver controls.
    // dt is currently unused; the simulation uses its own fixed timestep.
    // Call SimPlugin_Initialize before using this function.
    __declspec(dllexport) void SimPlugin_Update(double dt, double throttle, double brake, double steer) {
        if (!g_initialized) return;

        p0::input::InputState input;
        input.throttle = throttle;
        input.brake = brake;
        input.steering = steer;
        g_sim.step(input);
    }

    // Copy the current vehicle state into the caller-provided POD struct.
    // This is the primary read-back mechanism for the Unity/external integration.
    // Call SimPlugin_Initialize before using this function.
    __declspec(dllexport) void SimPlugin_GetVehicleState(VehicleState* state) {
        if (!state || !g_initialized) return;

        const auto& s = g_sim.state();
        state->x = s.position.x();
        state->y = s.position.y();
        state->vx = s.velocity.x();
        state->vy = s.velocity.y();
        state->heading = s.heading;
        state->speed = s.speed;
        state->rpm = s.rpm;
        state->gear = s.gear;
        state->throttle = s.throttle;
        state->brake = s.brake;
        state->steer = s.steer_angle;
    }
}
