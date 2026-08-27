// Project 0 — vehicle mesh generation tool
// Generates a parametric car mesh from default VehicleParams and exports
// it to both OBJ and GLB formats for use in the renderer and external tools.
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_generator.h"
#include "vehicle/mesh_exporter.h"
#include "vehicle/glb_exporter.h"
#include <iostream>

int main() {
    std::cout << "=== X-Racing Vehicle Generator ===" << std::endl;

    // Create default vehicle parameters (Porsche 911 style)
    p0::vehicle::VehicleParams params;

    std::cout << "\nVehicle Parameters:" << std::endl;
    std::cout << "  Mass: " << params.mass << " kg" << std::endl;
    std::cout << "  Wheelbase: " << params.wheelbase << " m" << std::endl;
    std::cout << "  Track Width: " << params.track_width << " m" << std::endl;
    std::cout << "  Max Power: " << params.max_power / 745.7 << " hp" << std::endl;
    std::cout << "  Max Torque: " << params.max_torque << " Nm" << std::endl;

    // Generate vehicle geometry
    p0::vehicle::VehicleGeometry geo = p0::vehicle::VehicleGenerator::FromParams(params);

    std::cout << "\nGenerated Geometry:" << std::endl;
    std::cout << "  Body Length: " << geo.body_length << " m" << std::endl;
    std::cout << "  Body Width: " << geo.body_width << " m" << std::endl;
    std::cout << "  Body Height: " << geo.body_height << " m" << std::endl;
    std::cout << "  Cabin Length: " << geo.cabin_length << " m" << std::endl;
    std::cout << "  Spoiler Height: " << geo.spoiler_height << " m" << std::endl;

    // Generate car mesh
    p0::vehicle::MeshData car = p0::vehicle::VehicleGenerator::GenerateCar(geo);

    std::cout << "\nGenerated Mesh:" << std::endl;
    std::cout << "  Vertices: " << car.vertices.size() << std::endl;
    std::cout << "  Triangles: " << car.indices.size() / 3 << std::endl;

    // Export to OBJ
    std::string objFilename = "D:/x-racing/data/models/vehicle.obj";
    p0::vehicle::MeshExporter::ExportOBJ(car, objFilename);

    // Export to GLB
    std::string glbFilename = "D:/x-racing/data/models/vehicle.glb";
    p0::vehicle::GLBExporter::ExportGLB(car, glbFilename);

    std::cout << "\nDone!" << std::endl;
    return 0;
}
