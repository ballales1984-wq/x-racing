#include "plugin/sim_plugin.h"
#include "simulation/simulation.h"
#include "track/track.h"
#include "vehicle/vehicle.h"
#include "input/input.h"

static p0::simulation::Simulation g_sim;
static bool g_initialized = false;

extern "C" {

    __declspec(dllexport) int SimPlugin_Initialize() {
        if (g_initialized) return 0;

        p0::track::Track track;
        g_sim.set_track(track);

        p0::vehicle::VehicleState initial;
        initial.position = track.get_start_position();
        initial.heading = track.get_start_heading();
        initial.speed = 0.0;

        g_sim.reset(initial);
        g_initialized = true;
        return 0;
    }

    __declspec(dllexport) void SimPlugin_Update(double dt, double throttle, double brake, double steer) {
        if (!g_initialized) return;

        p0::input::InputState input;
        input.throttle = throttle;
        input.brake = brake;
        input.steering = steer;
        g_sim.step(input);
    }

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
