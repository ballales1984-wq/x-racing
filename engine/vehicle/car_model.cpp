// X-Racing — car model registry implementation
// Author: alessio
#include "car_model.h"
#include "mesh_exporter.h"
#include "glb_exporter.h"
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace p0::vehicle {

//! @brief Returns the singleton instance of the car registry.
//! @reference car_model.h:33
CarRegistry& CarRegistry::instance() {
    static CarRegistry registry;
    return registry;
}

//! @brief Constructs the registry and initializes default car models.
CarRegistry::CarRegistry() {
    initialize_default_models();
}

//! @brief Initializes the default car models (Porsche 911 and Ferrari F12).
//!        Each model includes full vehicle physics parameters.
void CarRegistry::initialize_default_models() {
    models_.clear();
    id_to_index_.clear();

    // --- Car 1: Porsche 911 Turbo S (rear-engine) ---
    {
        CarModel model;
        model.id = "porsche_911";
        model.name = "Porsche 911 Turbo S";
        model.description = "Rear-engine sports car";
        model.author = "alessio";
        model.mesh_path = "data/models/porsche_911.obj";

        VehicleParams& p = model.params;
        // p keeps the struct defaults (Porsche 911 specs already defined)
        p.mass = 1500.0;
        p.wheelbase = 2.45;
        p.track_width = 1.6;
        p.cg_to_front = 1.4;   // rear-engine bias
        p.cg_to_rear = 1.05;
        p.max_power = 350000.0; // 470 hp
        p.max_torque = 530.0;
        p.max_rpm = 7200.0;
        p.final_drive = 3.44;
        p.gear_ratios = {3.67, 2.0, 1.35, 1.0, 0.85, 0.7};
        p.drag_coefficient = 0.33;
        p.frontal_area = 2.1;
        p.downforce_coefficient = 0.3;
        p.rear_wing_area = 0.3;
        p.tire_mu = 1.2;
        p.front_spring_rate = 35000.0;
        p.rear_spring_rate = 38000.0;

        model.geometry = VehicleGenerator::FromParams(p);
        register_model(model);
    }

    // --- Car 2: Ferrari F12berlinetta (mid-engine) ---
    {
        CarModel model;
        model.id = "ferrari_f12";
        model.name = "Ferrari F12berlinetta";
        model.description = "Mid-engine V12 supercar";
        model.author = "alessio";
        model.mesh_path = "data/models/ferrari_f12.obj";

        VehicleParams& p = model.params;
        p.mass = 1690.0;           // kg — heavier than 911
        p.wheelbase = 2.65;        // m — longer wheelbase
        p.track_width = 1.72;      // m — wider
        p.cg_height = 0.38;
        p.cg_to_front = 1.2;       // mid-engine, more balanced
        p.cg_to_rear = 1.45;
        p.wheel_radius = 0.33;
        p.max_steer_angle = 30.0 * kDegToRad;
        p.steer_ratio = 14.0;      // quicker steering
        p.max_power = 540000.0;    // ~720 hp
        p.max_torque = 690.0;      // Nm
        p.idle_rpm = 1000.0;
        p.max_rpm = 8500.0;        // higher redline
        p.final_drive = 3.18;
        p.engine_inertia = 0.22;
        p.drivetrain_loss = 0.10;
        p.gear_ratios = {3.31, 2.15, 1.54, 1.16, 0.91, 0.77, 0.62}; // 7-speed
        p.drag_coefficient = 0.34;
        p.frontal_area = 2.2;
        p.lift_coefficient = -0.15;
        p.downforce_coefficient = 0.45;
        p.aero_downforce_area = 1.8;
        p.ground_effect_factor = 0.04;
        p.front_wing_area = 0.12;
        p.rear_wing_area = 0.25;
        p.wing_angle = 0.12;       // more aggressive
        p.rolling_resistance = 0.012;
        p.max_brake_force = 22000.0;
        p.tire_mu = 1.35;          // stickier tires
        p.tire_pacejka_b = 12.0;
        p.tire_pacejka_c = 1.9;
        p.tire_pacejka_e = 0.97;
        p.tire_relaxation_length = 0.42;
        p.tire_camber_gain = 1.1;
        p.tire_load_sensitivity = 0.06;
        p.tire_max_reference_load = 5000.0;
        p.front_spring_rate = 40000.0;   // stiffer
        p.rear_spring_rate = 42000.0;
        p.front_damping = 4000.0;
        p.rear_damping = 4200.0;
        p.ride_height = 0.10;
        p.anti_roll_bar_stiffness = 14000.0;
        p.max_body_roll = 0.07;
        p.max_body_pitch = 0.05;
        p.roll_damping = 0.88;
        p.pitch_damping = 0.88;
        p.camber_gain_per_roll = -3.5;
        p.ambient_temperature = 293.0;     // K, 20°C
        p.tire_optimal_temp = 343.0;       // K, 70°C
        p.tire_temp_curve_width = 22.0;
        p.tire_cooling_rate = 0.45;
        p.tire_heat_per_slip = 1.6;
        p.tire_wear_per_slip = 0.00009;
        p.tire_wear_per_lap = 0.0009;
        p.rain_intensity = 0.0;
        p.rain_grip_reduction = 0.35;
        p.rain_rolling_resistance = 1.2;
        p.wind_effect_on_speed = 0.08;
        p.temp_cooling_rate = 0.16;
        p.rain_cooling = 1.6;
        p.track_heat_rate = 0.045;

        model.geometry = VehicleGenerator::FromParams(p);
        register_model(model);
    }
}

//! @brief Registers a car model in the registry.
//!        If the model ID already exists, updates the existing entry.
//! @param model The car model to register.
void CarRegistry::register_model(const CarModel& model) {
    auto it = id_to_index_.find(model.id);
    if (it != id_to_index_.end()) {
        models_[it->second] = model;
        return;
    }
    id_to_index_[model.id] = models_.size();
    models_.push_back(model);
}

//! @brief Looks up a car model by its ID.
//! @param id The model identifier.
//! @return Pointer to the model, or nullptr if not found.
const CarModel* CarRegistry::get(const std::string& id) const {
    auto it = id_to_index_.find(id);
    if (it == id_to_index_.end()) {
        return nullptr;
    }
    return &models_[it->second];
}

//! @brief Returns all registered car models.
//! @return Const reference to the models vector.
const std::vector<CarModel>& CarRegistry::all() const {
    return models_;
}

//! @brief Returns the number of registered models.
//! @return Count of models.
size_t CarRegistry::size() const {
    return models_.size();
}

//! @brief Checks if a model with the given ID exists.
//! @param id The model identifier.
//! @return true if the model exists.
bool CarRegistry::has(const std::string& id) const {
    return id_to_index_.find(id) != id_to_index_.end();
}

//! @brief Generates the mesh for a car model and exports it to OBJ and GLB.
//!        Derives geometry from physics parameters.
//! @param model The car model to generate mesh for.
//! @return true if mesh generation and export succeeded.
bool CarRegistry::generate_mesh(CarModel& model) {
    model.geometry = VehicleGenerator::FromParams(model.params);

    std::string path = model.mesh_path;
    std::filesystem::path fs_path(path);
    if (!path.empty() && fs_path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(fs_path.parent_path(), ec);
    }

    MeshData mesh = VehicleGenerator::GenerateCar(model.geometry);

    if (path.empty() || !MeshExporter::ExportOBJ(mesh, path)) {
        return false;
    }

    // Also export GLB alongside OBJ
    std::filesystem::path glb_path = fs_path;
    if (!glb_path.has_extension()) glb_path += ".glb";
    else glb_path.replace_extension(".glb");
    GLBExporter::ExportGLB(mesh, glb_path.string());

    return true;
}

//! @brief Generates meshes for all registered models.
//!        Optionally overrides the output directory.
//! @param output_dir Directory to save meshes (empty = use model's mesh_path).
//! @return true if all meshes were generated successfully.
bool CarRegistry::generate_all_meshes(const std::string& output_dir) {
    bool ok = true;
    for (auto& model : models_) {
        std::string original_path = model.mesh_path;

        if (!output_dir.empty()) {
            std::filesystem::path dir(output_dir);
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            std::filesystem::path p = dir / (model.id + ".obj");
            model.mesh_path = p.string();
        }

        std::cout << "Generating mesh for: " << model.name << " (" << model.id << ")" << std::endl;
        if (!generate_mesh(model)) {
            std::cerr << "  FAILED to generate mesh for " << model.id << std::endl;
            ok = false;
        } else {
            model.mesh_path = original_path;
        }
    }
    return ok;
}

} // namespace p0::vehicle