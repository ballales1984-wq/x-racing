// X-Racing — OBJ mesh exporter
// Author: alessio
#pragma once

#include "vehicle/vehicle_generator.h"
#include <fstream>
#include <string>

namespace p0::vehicle {

// Project 0 — OBJ mesh exporter
// Namespace: p0::vehicle

// Export mesh to Wavefront OBJ format (compatible with Unity, Blender, etc.).
class MeshExporter {
 public:
    // Export a Mesh directly to an OBJ file.
    static bool ExportOBJ(const MeshData& mesh, const std::string& filename);
    // Generate a car mesh from geometry and export it to OBJ.
    static bool ExportCarOBJ(const VehicleGeometry& geo, const std::string& filename);
};

} // namespace p0::vehicle
