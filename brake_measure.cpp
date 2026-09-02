#include <iostream>
#include <cmath>
#include "engine/vehicle/vehicle.h"
#include "engine/simulation/simulation.h"
#include "engine/track/track.h"
#include "engine/input/input.h"
#include "engine/physics/types.h"
#include "engine/physics/tire_model.h"

using namespace p0;

int main() {
    track::Track track;
    simulation::Simulation sim;
    sim.set_track(track);

    vehicle::VehicleState initial;
    initial.position = track.get_start_position();
    initial.heading = track.get_start_heading();
    initial.speed = 100.0 / 3.6;
    initial.front_tire_temp = vehicle::VehicleParams{}.tire_optimal_temp;
    initial.rear_tire_temp = vehicle::VehicleParams{}.tire_optimal_temp;
    sim.reset(initial);

    input::InputState input;
    input.brake = 1.0;

    double distance_traveled = 0.0;
    const double dt = 1.0 / 60.0;
    const double v0 = initial.speed;
    double peak_decel = 0.0;
    int steps = 0;

    std::cout << "Starting speed: " << v0 * 3.6 << " km/h\n";
    std::cout << "ambient=" << vehicle::VehicleParams{}.ambient_temperature
              << " optimal=" << vehicle::VehicleParams{}.tire_optimal_temp << "\n";

    for (int i = 0; i < 2000; ++i) {
        double prev_speed = sim.state().speed;
        sim.step(input);
        double curr_speed = sim.state().speed;
        distance_traveled += curr_speed * dt;
        double decel = (prev_speed - curr_speed) / dt;
        if (decel > peak_decel) peak_decel = decel;
        steps++;
        if (curr_speed < 0.5) break;
    }

    std::cout << "Steps: " << steps << "\n";
    std::cout << "Stopping distance: " << distance_traveled << " m\n";
    std::cout << "Peak deceleration: " << peak_decel << " m/s^2 (";
    std::cout << peak_decel / 9.80665 << " G)\n";
    std::cout << "Final speed: " << sim.state().speed * 3.6 << " km/h\n";
    std::cout << "Final front_tire_temp: " << sim.state().front_tire_temp << " K\n";
    std::cout << "Final rear_tire_temp: " << sim.state().rear_tire_temp << " K\n";
    std::cout << "Final tire_wear front: " << sim.state().front_tire_wear << "\n";

    double tp_grip = vehicle::VehicleParams{}.tire_mu * 1.0 * 1.0;
    std::cout << "Theoretical max decel (cold mu*g): " << tp_grip * 9.80665 << " m/s^2\n";

    return 0;
}
