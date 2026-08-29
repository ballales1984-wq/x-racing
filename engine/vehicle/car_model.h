// X-Racing — vehicle subsystem
// Author: alessio
#pragma once

#include "vehicle/vehicle.h"
#include "vehicle/vehicle_generator.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace p0::vehicle {

// CarModel: "specification card" of a single car model.
// Binds together identity metadata, physical parameters, derived geometry,
// and the path to the exported visual mesh asset — forming a single
// cohesive definition that can be identified, looked up and attached
// throughout the simulation, renderer and gameplay layers.
struct CarModel {
    std::string id;            // machine identifier, e.g. "porsche_911"
    std::string name;          // human-readable name, e.g. "Porsche 911 Turbo S"
    std::string description;   // optional description / body style
    std::string author;        // author / creator of this car model definition
    std::string mesh_path;     // path to exported OBJ/GLB visual mesh
    VehicleParams params;      // physical parameters
    VehicleGeometry geometry;  // derived geometry (filled from params)
};

// CarRegistry: repository of all predefined car models ("specification cards") available
// in the simulation.  Models are identified by their unique string id and
// linked to their visual mesh asset via mesh_path.
class CarRegistry {
public:
    // Singleton access point.
    static CarRegistry& instance();

    // Register a new car model.  The model id must be unique.
    void register_model(const CarModel& model);

    // Look up a car model by id.  Returns nullptr if not found.
    const CarModel* get(const std::string& id) const;

    // All registered car models.
    const std::vector<CarModel>& all() const;

    // Number of registered models.
    size_t size() const;

    // Derive geometry from params for a model, generate the mesh and
    // export it to the model's mesh_path as OBJ.
    // Returns true on success.
    bool generate_mesh(CarModel& model);

    // Generate and export meshes for every registered model.
    // output_dir overrides mesh_path if non-empty; otherwise each
    // model's mesh_path is used directly.
    bool generate_all_meshes(const std::string& output_dir = "");

    // Check whether a model with the given id exists.
    bool has(const std::string& id) const;

private:
    CarRegistry();
    void initialize_default_models();

    std::vector<CarModel> models_;
    std::unordered_map<std::string, size_t> id_to_index_;
};

} // namespace p0::vehicle
