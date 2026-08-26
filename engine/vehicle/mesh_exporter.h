#pragma once

#include "vehicle/vehicle_generator.h"
#include <fstream>
#include <string>

namespace p0::vehicle {

// Export mesh to OBJ format (compatible with Unity, Blender, etc.)
class MeshExporter {
public:
    static bool ExportOBJ(const MeshData& mesh, const std::string& filename);
    static bool ExportCarOBJ(const VehicleGeometry& geo, const std::string& filename);
};

} // namespace p0::vehicle
