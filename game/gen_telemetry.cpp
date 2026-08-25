#include "simulation/simulation.h"
#include "track/track.h"
#include "vehicle/vehicle.h"
#include "input/input.h"
#include "telemetry/telemetry.h"
#include <iostream>
#include <cmath>

int main() {
    p0::track::Track track;
    p0::simulation::Simulation sim;
    p0::telemetry::Telemetry tel;

    sim.set_track(track);

    p0::vehicle::VehicleState initial;
    initial.position = track.get_start_position();
    initial.heading = track.get_start_heading();
    initial.speed = 0.0;

    sim.reset(initial);
    tel.clear();

    const double dt = 1.0 / 60.0;
    const int total_frames = 60 * 60;

    for (int frame = 0; frame < total_frames; frame++) {
        p0::input::InputState input = {};

        double t = frame * dt;
        double distance = sim.state().distance_along_track;
        auto tp = track.at(distance);

        input.throttle = 0.7;
        input.brake = 0.0;
        input.steering = 0.0;

        double lookahead_dist = distance + 50.0;
        auto future_tp = track.at(lookahead_dist);
        double desired_heading = std::atan2(future_tp.tangent.y(), future_tp.tangent.x());
        double heading_error = desired_heading - sim.state().heading;
        while (heading_error > 3.14159) heading_error -= 2.0 * 3.14159;
        while (heading_error < -3.14159) heading_error += 2.0 * 3.14159;
        input.steering = std::max(-1.0, std::min(1.0, heading_error * 2.0));

        if (tp.curvature > 0.01) {
            input.throttle = 0.4;
            if (sim.state().speed > 30.0) input.brake = 0.3;
        } else if (tp.curvature < -0.01) {
            input.throttle = 0.5;
            if (sim.state().speed > 35.0) input.brake = 0.2;
        }

        if (sim.state().speed > 80.0) {
            input.throttle = 0.3;
        }

        auto result = sim.step(input);
        tel.record(result.state, dt);
    }

    tel.save_csv("D:/x-racing/data/telemetry/unity_state.csv");
    std::cout << "Telemetry generated: " << total_frames << " frames" << std::endl;
    std::cout << "Final distance: " << sim.state().distance_along_track << " m" << std::endl;
    std::cout << "Laps: " << sim.state().lap << std::endl;

    return 0;
}
