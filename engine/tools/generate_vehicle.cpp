// Project 0 — vehicle mesh generation tool
// Generates parametric car meshes for every model in the CarRegistry and
// exports them to OBJ + GLB for use in the renderer and external tools.
#include "vehicle/car_model.h"
#include "vehicle/vehicle_generator.h"
#include "vehicle/mesh_exporter.h"
#include "vehicle/glb_exporter.h"
#include <iostream>

int main() {
    std::cout << "=== X-Racing Vehicle Generator ===" << std::endl;

    // Access the car registry
    auto& registry = p0::vehicle::CarRegistry::instance();

    std::cout << "\nAvailable car models: " << registry.size() << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // Print a summary sheet ("scheda") for each car
    for (const auto& model : registry.all()) {
        std::cout << "\nCar: " << model.name << " (id: " << model.id << ")" << std::endl;
        std::cout << "  Description: " << model.description << std::endl;
        std::cout << "  Mass:       " << model.params.mass << " kg" << std::endl;
        std::cout << "  Wheelbase:  " << model.params.wheelbase << " m" << std::endl;
        std::cout << "  Track:      " << model.params.track_width << " m" << std::endl;
        std::cout << "  Max Power:  " << model.params.max_power / 745.7 << " hp" << std::endl;
        std::cout << "  Max Torque: " << model.params.max_torque << " Nm" << std::endl;
        std::cout << "  Max RPM:    " << model.params.max_rpm << std::endl;
        std::cout << "  Gears:      " << model.params.gear_ratios.size() << std::endl;
        std::cout << "  Drag Cd:    " << model.params.drag_coefficient << std::endl;
        std::cout << "  Tire Mu:    " << model.params.tire_mu << std::endl;
        std::cout << "  Mesh path:  " << model.mesh_path << std::endl;

        std::cout << "\n  Geometry:" << std::endl;
        std::cout << "    Body L:   " << model.geometry.body_length << " m" << std::endl;
        std::cout << "    Body W:   " << model.geometry.body_width << " m" << std::endl;
        std::cout << "    Body H:   " << model.geometry.body_height << " m" << std::endl;
        std::cout << "    Wheel R:  " << model.geometry.wheel_radius << " m" << std::endl;
        std::cout << "    Spoiler H:" << model.geometry.spoiler_height << " m" << std::endl;
    }

    std::cout << "\n----------------------------------------" << std::endl;

    // Generate and export meshes for every registered car model
    std::string output_dir = "D:/x-racing/data/models/";
    bool ok = registry.generate_all_meshes(output_dir);

    std::cout << "\nDone!" << std::endl;
    std::cout << "  Car models processed: " << registry.size() << std::endl;

    return ok ? 0 : 1;
}
