// Project 0 — telemetry generation tool
// Runs a scripted AI driver around the track and records telemetry data
// for offline analysis and Unity integration.
#include "simulation/simulation.h"
#include "track/track.h"
#include "vehicle/vehicle.h"
#include "input/input.h"
#include "telemetry/telemetry.h"
#include "common.h"
#include <iostream>
#include <cmath>

using namespace p0;

int main() {
    p0::track::Track track;
    p0::simulation::Simulation sim;
    p0::telemetry::Telemetry tel;

    sim.set_track(track);

    // Initialize the car at the start line.
    p0::vehicle::VehicleState initial;
    initial.position = track.get_start_position();
    initial.heading = track.get_start_heading();
    initial.speed = 0.0;

    sim.reset(initial);
    tel.clear();

    const double dt = 1.0 / 60.0;
    // Run for 5 minutes of simulated time so the AI has time to complete
    // multiple laps and produce useful lap-time data in the CSV.
    const int total_frames = 60 * 60 * 5;

    double current_lap_time = 0.0;
    double last_lap_time = 0.0;
    int last_recorded_lap = 0;

    for (int frame = 0; frame < total_frames; frame++) {
        p0::input::InputState input = {};

        double t = frame * dt;
        double distance = sim.state().distance_along_track;
        auto tp = track.at(distance);

        // Default driving state: full throttle, no braking.
        input.throttle = 1.0;
        input.brake = 0.0;
        input.steering = 0.0;

        // Lookahead steering: aim at a point along the track ahead. Scale
        // the lookahead with speed so faster cars plan further ahead; this
        // keeps the steering smooth and avoids saturating at the start of
        // a curve where the heading error is large.
        const double speed = sim.state().speed;
        const double lookahead_dist = distance + std::max(25.0, speed * 0.6);
        auto future_tp = track.at(lookahead_dist);
        double desired_heading = std::atan2(future_tp.tangent.y(), future_tp.tangent.x());
        double heading_error = desired_heading - sim.state().heading;
        while (heading_error > kPi) heading_error -= kTwoPi;
        while (heading_error < -kPi) heading_error += kTwoPi;
        // Gentler steering gain: a saturated steering at the start of every
        // curve is what makes the previous profile spin out into the barrier.
        input.steering = std::max(-1.0, std::min(1.0, heading_error * 0.6));

        // Cornering speed control: ease off the throttle in curves instead of
        // braking hard, which previously stalled the car on the second straight.
        const double abs_curv = std::abs(tp.curvature);
        if (abs_curv > 0.005) {
            // Target corner speed scales with the inverse square root of curvature.
            const double target_corner_speed = std::min(60.0, 25.0 / std::sqrt(abs_curv + 0.01));
            if (speed > target_corner_speed) {
                input.throttle = 0.0;
                input.brake = 0.4;
            } else {
                input.throttle = 0.6;
            }
        }

        auto result = sim.step(input);
        tel.record(result.state, dt);
        current_lap_time += dt;

        if (result.state.lap > last_recorded_lap) {
            last_lap_time = current_lap_time;
            tel.mark_lap(result.state.lap);
            std::cout << "Lap " << result.state.lap
                      << " completed: " << last_lap_time << " s"
                      << " (avg " << (track.length() / last_lap_time) * 3.6 << " km/h)"
                      << std::endl;
            current_lap_time = 0.0;
            last_recorded_lap = result.state.lap;
        }
    }

    // Save telemetry to CSV for external analysis.
    tel.save_csv("data/telemetry/unity_state.csv");
    std::cout << "\nTelemetry generated: " << total_frames << " frames" << std::endl;
    std::cout << "Final distance: " << sim.state().distance_along_track << " m" << std::endl;
    std::cout << "Laps: " << sim.state().lap << std::endl;
    if (last_lap_time > 0.0) {
        std::cout << "Last lap time: " << last_lap_time << " s" << std::endl;
    }
    std::cout << "Saved to data/telemetry/unity_state.csv" << std::endl;

    return 0;
}
