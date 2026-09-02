#include "plugin/sim_plugin.h"
#include "simulation/simulation.h"
#include "track/track.h"
#include "vehicle/vehicle.h"
#include "input/input.h"
#include "tracking/tracking_system.h"
#include "tracking/simulated_gps.h"
#include "tracking/physics_trajectory.h"
#include "tracking/coordinate_converter.h"
#include "tracking/track_mapper.h"
#include <mutex>

// Global simulation instance used by the C plugin ABI.
// This singleton is shared across all DLL calls and persists for the
// lifetime of the loaded module.
// NOTE: Simulation stores a pointer to the track, so the track must outlive
// the simulation; keep it as a persistent static rather than a local.
// Thread safety: all public functions acquire g_mutex before accessing state.
static p0::track::Track g_track;
static p0::simulation::Simulation g_sim;
static bool g_initialized = false;
static std::mutex g_mutex;

// Tracking layer: a simulated GPS fed by the live vehicle state, mapped onto
// the track. Exposed to Unity via SimPlugin_GetTracking.
static std::unique_ptr<p0::tracking::PhysicsTrajectory> g_trajectory_owner;
static p0::tracking::PhysicsTrajectory* g_trajectory_raw = nullptr;
static std::unique_ptr<p0::tracking::SimulatedGPS> g_gps_owner;
static p0::tracking::SimulatedGPS* g_gps_raw = nullptr;
static std::unique_ptr<p0::tracking::TrackingSystem> g_tracking;

extern "C" {

    // Initialize the simulation: build a default track and reset the car
    // to the start line. Safe to call once; subsequent calls are ignored.
    // Returns 0 on success.
    __declspec(dllexport) int SimPlugin_Initialize() {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_initialized) return 0;

        g_track = p0::track::Track();
        g_sim.set_track(g_track);

        p0::vehicle::VehicleState initial;
        initial.position = g_track.get_start_position();
        initial.heading = g_track.get_start_heading();
        initial.speed = 0.0;

        g_sim.reset(initial);
        g_initialized = true;

        g_trajectory_owner = std::make_unique<p0::tracking::PhysicsTrajectory>(
            p0::tracking::CoordinateConverter(p0::tracking::GeographicOrigin{45.0, 11.0, 0.0}),
            &g_sim.state());
        g_trajectory_raw = g_trajectory_owner.get();

        g_gps_owner = std::make_unique<p0::tracking::SimulatedGPS>(
            std::move(g_trajectory_owner),
            p0::tracking::SimulatedGPS::Params{10.0});
        g_gps_raw = g_gps_owner.get();
        g_tracking = std::make_unique<p0::tracking::TrackingSystem>(
            std::move(g_gps_owner));
        g_tracking->set_mapper(
            std::make_unique<p0::tracking::TrackMapper>(g_track, p0::tracking::TrackMapper::LocalOrigin{45.0, 11.0}));
        g_tracking->start();
        return 0;
    }

    // Advance the simulation by one step using the provided driver controls.
    // dt is currently unused; the simulation uses its own fixed timestep.
    // Call SimPlugin_Initialize before using this function.
    __declspec(dllexport) void SimPlugin_Update(double dt, double throttle, double brake, double steer) {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_initialized) return;

        p0::input::InputState input;
        input.throttle = throttle;
        input.brake = brake;
        input.steering = steer;
        g_sim.step(input);

        if (g_tracking) {
            g_tracking->update();
        }
    }

    // Copy the current vehicle state into the caller-provided POD struct.
    // This is the primary read-back mechanism for the Unity/external integration.
    // Call SimPlugin_Initialize before using this function.
    __declspec(dllexport) void SimPlugin_GetVehicleState(VehicleState* state) {
        if (!state) return;
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_initialized) return;

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

    // Geographic + track-relative snapshot from the tracking layer.
    // Returns 0 on success; -1 if the tracking layer is not ready.
    __declspec(dllexport) int SimPlugin_GetTracking(TrackingSample* out) {
        if (!out) return -1;
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_initialized || !g_tracking) return -1;

        const auto& sample = g_tracking->current_sample();
        const auto& tp = g_tracking->current_track_position();

        out->latitude = sample.latitude;
        out->longitude = sample.longitude;
        out->altitude = sample.altitude;
        out->track_s = tp.s;
        out->track_l = tp.lateral;
        out->speed = sample.speed;
        out->heading = sample.heading;
        out->accuracy = sample.horizontal_accuracy;
        out->on_track = tp.on_track ? 1 : 0;
        return 0;
    }

    // Release all resources held by the plugin. Safe to call multiple times.
    // After this call, SimPlugin_Initialize must be called again before use.
    __declspec(dllexport) void SimPlugin_Shutdown() {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_tracking) {
            g_tracking->stop();
        }
        g_tracking.reset();
        g_gps_owner.reset();
        g_trajectory_owner.reset();
        g_initialized = false;
    }

}
