#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include <vector>
#include <cmath>

namespace p0::vehicle {

// Mesh data structure
struct MeshData {
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<int> indices;
};

// Vehicle geometry parameters derived from VehicleParams
struct VehicleGeometry {
    double body_length = 4.5;
    double body_width = 1.85;
    double body_height = 1.15;
    double cabin_length = 2.2;
    double cabin_width = 1.6;
    double cabin_height = 1.0;
    double wheelbase = 2.45;
    double track_width = 1.6;
    double wheel_radius = 0.33;
    double wheel_width = 0.25;
    double front_overhang = 1.0;
    double rear_overhang = 1.05;
    double ride_height = 0.12;
    double splitter_height = 0.05;
    double splitter_depth = 0.15;
    double spoiler_height = 0.6;
    double spoiler_width = 1.4;
    double spoiler_thickness = 0.08;
    double hood_slope = 0.15;
    double windshield_angle = 0.4;
    double rear_window_angle = 0.3;
};

class VehicleGenerator {
public:
    static VehicleGeometry FromParams(const VehicleParams& params);

    // Generate complete car mesh
    static MeshData GenerateCar(const VehicleGeometry& geo);

    // Generate individual components
    static MeshData GenerateBody(const VehicleGeometry& geo);
    static MeshData GenerateCabin(const VehicleGeometry& geo);
    static MeshData GenerateWheel(const VehicleGeometry& geo);
    static MeshData GenerateSpoiler(const VehicleGeometry& geo);
    static MeshData GenerateSplitter(const VehicleGeometry& geo);
    static MeshData GenerateSideMirrors(const VehicleGeometry& geo);
    static MeshData GenerateHeadlights(const VehicleGeometry& geo);
    static MeshData GenerateTaillights(const VehicleGeometry& geo);

private:
    static void AddBox(MeshData& mesh, const Vec3& center, const Vec3& size);
    static void AddCylinder(MeshData& mesh, const Vec3& center, double radius, double height, int segments);
    static void MergeMesh(MeshData& target, const MeshData& source);
};

} // namespace p0::vehicle
