// X-Racing — GLB (binary glTF) mesh exporter
// Author: alessio
#pragma once

#include "vehicle/vehicle_generator.h"
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

namespace p0::vehicle {

// Project 0 — GLB (binary glTF) mesh exporter
// Namespace: p0::vehicle

// Export mesh to GLB (binary glTF) format for use in engines and 3D tools.
class GLBExporter {
public:
    // Export a Mesh directly to a GLB file.
    static bool ExportGLB(const MeshData& mesh, const std::string& filename);
    // Generate a car mesh from geometry and export it to GLB.
    static bool ExportCarGLB(const VehicleGeometry& geo, const std::string& filename);

private:
    // Pack mesh data into a flat binary buffer (positions, normals, indices, UVs).
    static std::vector<uint8_t> CreateBinaryBuffer(const MeshData& mesh);
    // Build a minimal glTF 2.0 JSON description for the mesh.
    static std::string CreateGLTFJson(const MeshData& mesh, size_t bufferOffset, size_t bufferSize);
    // Append a little-endian uint32 to the buffer.
    static void WriteUint32(std::vector<uint8_t>& buffer, uint32_t value);
    // Pad the buffer to the given byte alignment.
    static void AlignBuffer(std::vector<uint8_t>& buffer, uint32_t alignment);
};

} // namespace p0::vehicle
