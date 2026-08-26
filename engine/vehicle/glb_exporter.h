#pragma once

#include "vehicle/vehicle_generator.h"
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

namespace p0::vehicle {

// Export mesh to GLB (binary glTF) format
class GLBExporter {
public:
    static bool ExportGLB(const MeshData& mesh, const std::string& filename);
    static bool ExportCarGLB(const VehicleGeometry& geo, const std::string& filename);

private:
    static std::vector<uint8_t> CreateBinaryBuffer(const MeshData& mesh);
    static std::string CreateGLTFJson(const MeshData& mesh, size_t bufferOffset, size_t bufferSize);
    static void WriteUint32(std::vector<uint8_t>& buffer, uint32_t value);
    static void AlignBuffer(std::vector<uint8_t>& buffer, uint32_t alignment);
};

} // namespace p0::vehicle
