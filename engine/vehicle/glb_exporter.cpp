#include "glb_exporter.h"
#include <iostream>
#include <cstring>
#include <algorithm>

// Project 0 — GLB exporter implementation
// Writes a minimal glTF 2.0 binary file with PBR material.
namespace p0::vehicle {

// Export a Mesh to a binary GLB file.
// The file contains: 12-byte GLB header, JSON chunk, binary chunk.
bool GLBExporter::ExportGLB(const MeshData& mesh, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return false;
    }

    // Create binary buffer
    std::vector<uint8_t> binBuffer = CreateBinaryBuffer(mesh);

    // Create JSON
    std::string json = CreateGLTFJson(mesh, 0, binBuffer.size());

    // Pad JSON to 4-byte boundary
    while (json.size() % 4 != 0) {
        json += ' ';
    }

    // GLB header
    uint32_t magic = 0x46546C67; // "glTF"
    uint32_t version = 2;
    uint32_t totalLength = 12 + 8 + json.size() + 8 + binBuffer.size();

    // Write header
    file.write(reinterpret_cast<const char*>(&magic), 4);
    file.write(reinterpret_cast<const char*>(&version), 4);
    file.write(reinterpret_cast<const char*>(&totalLength), 4);

    // Write JSON chunk
    uint32_t jsonChunkLength = json.size();
    uint32_t jsonChunkType = 0x4E4F534A; // "JSON"
    file.write(reinterpret_cast<const char*>(&jsonChunkLength), 4);
    file.write(reinterpret_cast<const char*>(&jsonChunkType), 4);
    file.write(json.data(), json.size());

    // Write binary chunk
    uint32_t binChunkLength = binBuffer.size();
    uint32_t binChunkType = 0x004E4942; // "BIN\0"
    file.write(reinterpret_cast<const char*>(&binChunkLength), 4);
    file.write(reinterpret_cast<const char*>(&binChunkType), 4);
    file.write(reinterpret_cast<const char*>(binBuffer.data()), binBuffer.size());

    file.close();
    std::cout << "Exported GLB to: " << filename << "\n";
    std::cout << "  Vertices: " << mesh.vertices.size() << "\n";
    std::cout << "  Triangles: " << mesh.indices.size() / 3 << "\n";
    std::cout << "  File size: " << totalLength << " bytes\n";

    return true;
}

// Generate a complete car mesh and export it to GLB.
bool GLBExporter::ExportCarGLB(const VehicleGeometry& geo, const std::string& filename) {
    MeshData car = VehicleGenerator::GenerateCar(geo);
    return ExportGLB(car, filename);
}

// Pack mesh vertex attributes into a flat interleaved binary buffer.
// Layout: positions (float32 x3), normals (float32 x3), indices (uint32), UVs (float32 x2).
std::vector<uint8_t> GLBExporter::CreateBinaryBuffer(const MeshData& mesh) {
    std::vector<uint8_t> buffer;

    // Calculate bounds for position data
    double minX = 1e10, minY = 1e10, minZ = 1e10;
    double maxX = -1e10, maxY = -1e10, maxZ = -1e10;
    for (const auto& v : mesh.vertices) {
        minX = std::min(minX, v[0]);
        minY = std::min(minY, v[1]);
        minZ = std::min(minZ, v[2]);
        maxX = std::max(maxX, v[0]);
        maxY = std::max(maxY, v[1]);
        maxZ = std::max(maxZ, v[2]);
    }

    // Position data (float32, 3 components)
    uint32_t positionOffset = buffer.size();
    for (const auto& v : mesh.vertices) {
        float x = static_cast<float>(v[0]);
        float y = static_cast<float>(v[1]);
        float z = static_cast<float>(v[2]);
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&x);
        buffer.insert(buffer.end(), ptr, ptr + 4);
        ptr = reinterpret_cast<uint8_t*>(&y);
        buffer.insert(buffer.end(), ptr, ptr + 4);
        ptr = reinterpret_cast<uint8_t*>(&z);
        buffer.insert(buffer.end(), ptr, ptr + 4);
    }

    // Normal data (float32, 3 components)
    uint32_t normalOffset = buffer.size();
    for (const auto& n : mesh.normals) {
        float nx = static_cast<float>(n[0]);
        float ny = static_cast<float>(n[1]);
        float nz = static_cast<float>(n[2]);
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&nx);
        buffer.insert(buffer.end(), ptr, ptr + 4);
        ptr = reinterpret_cast<uint8_t*>(&ny);
        buffer.insert(buffer.end(), ptr, ptr + 4);
        ptr = reinterpret_cast<uint8_t*>(&nz);
        buffer.insert(buffer.end(), ptr, ptr + 4);
    }

    // Index data (uint32)
    uint32_t indexOffset = buffer.size();
    for (int idx : mesh.indices) {
        uint32_t index = static_cast<uint32_t>(idx);
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&index);
        buffer.insert(buffer.end(), ptr, ptr + 4);
    }

    // UV data (float32, 2 components)
    uint32_t uvOffset = buffer.size();
    for (const auto& uv : mesh.uvs) {
        float u = static_cast<float>(uv[0]);
        float v = static_cast<float>(uv[1]);
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&u);
        buffer.insert(buffer.end(), ptr, ptr + 4);
        ptr = reinterpret_cast<uint8_t*>(&v);
        buffer.insert(buffer.end(), ptr, ptr + 4);
    }

    // Pad to 4-byte boundary
    AlignBuffer(buffer, 4);

    return buffer;
}

// Build a minimal glTF 2.0 JSON describing the mesh and buffer views.
std::string GLBExporter::CreateGLTFJson(const MeshData& mesh, size_t bufferOffset, size_t bufferSize) {
    // Calculate bounds
    double minX = 1e10, minY = 1e10, minZ = 1e10;
    double maxX = -1e10, maxY = -1e10, maxZ = -1e10;
    for (const auto& v : mesh.vertices) {
        minX = std::min(minX, v[0]);
        minY = std::min(minY, v[1]);
        minZ = std::min(minZ, v[2]);
        maxX = std::max(maxX, v[0]);
        maxY = std::max(maxY, v[1]);
        maxZ = std::max(maxZ, v[2]);
    }

    // Calculate byte offsets for each buffer view
    size_t positionByteOffset = 0;
    size_t normalByteOffset = mesh.vertices.size() * 12; // 3 floats * 4 bytes
    size_t indexByteOffset = normalByteOffset + mesh.normals.size() * 12;
    size_t uvByteOffset = indexByteOffset + mesh.indices.size() * 4;

    std::string json = R"({
  "asset": {
    "version": "2.0",
    "generator": "X-Racing Vehicle Generator"
  },
  "scene": 0,
  "scenes": [
    {
      "nodes": [0]
    }
  ],
  "nodes": [
    {
      "mesh": 0,
      "name": "Vehicle"
    }
  ],
  "meshes": [
    {
      "primitives": [
        {
          "attributes": {
            "POSITION": 0,
            "NORMAL": 1,
            "TEXCOORD_0": 3
          },
          "indices": 2,
          "material": 0
        }
      ]
    }
  ],
  "materials": [
    {
      "pbrMetallicRoughness": {
        "baseColorFactor": [0.9, 0.1, 0.1, 1.0],
        "metallicFactor": 0.5,
        "roughnessFactor": 0.5
      },
      "name": "CarBody"
    }
  ],
  "buffers": [
    {
      "byteLength": )" + std::to_string(bufferSize) + R"(
    }
  ],
  "bufferViews": [
    {
      "buffer": 0,
      "byteOffset": )" + std::to_string(positionByteOffset) + R"(,
      "byteLength": )" + std::to_string(mesh.vertices.size() * 12) + R"(,
      "target": 34962
    },
    {
      "buffer": 0,
      "byteOffset": )" + std::to_string(normalByteOffset) + R"(,
      "byteLength": )" + std::to_string(mesh.normals.size() * 12) + R"(,
      "target": 34962
    },
    {
      "buffer": 0,
      "byteOffset": )" + std::to_string(indexByteOffset) + R"(,
      "byteLength": )" + std::to_string(mesh.indices.size() * 4) + R"(,
      "target": 34963
    },
    {
      "buffer": 0,
      "byteOffset": )" + std::to_string(uvByteOffset) + R"(,
      "byteLength": )" + std::to_string(mesh.uvs.size() * 8) + R"(,
      "target": 34962
    }
  ],
  "accessors": [
    {
      "bufferView": 0,
      "componentType": 5126,
      "count": )" + std::to_string(mesh.vertices.size()) + R"(,
      "type": "VEC3",
      "min": [)" + std::to_string(minX) + ", " + std::to_string(minY) + ", " + std::to_string(minZ) + R"(],
      "max": [)" + std::to_string(maxX) + ", " + std::to_string(maxY) + ", " + std::to_string(maxZ) + R"(]
    },
    {
      "bufferView": 1,
      "componentType": 5126,
      "count": )" + std::to_string(mesh.normals.size()) + R"(,
      "type": "VEC3"
    },
    {
      "bufferView": 2,
      "componentType": 5125,
      "count": )" + std::to_string(mesh.indices.size()) + R"(,
      "type": "SCALAR"
    },
    {
      "bufferView": 3,
      "componentType": 5126,
      "count": )" + std::to_string(mesh.uvs.size()) + R"(,
      "type": "VEC2"
    }
  ]
})";

    return json;
}

// Append a 32-bit unsigned integer in little-endian byte order.
void GLBExporter::WriteUint32(std::vector<uint8_t>& buffer, uint32_t value) {
    uint8_t bytes[4];
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
    bytes[3] = (value >> 24) & 0xFF;
    buffer.insert(buffer.end(), bytes, bytes + 4);
}

// Pad the buffer with zero bytes until its size is a multiple of alignment.
void GLBExporter::AlignBuffer(std::vector<uint8_t>& buffer, uint32_t alignment) {
    while (buffer.size() % alignment != 0) {
        buffer.push_back(0);
    }
}

} // namespace p0::vehicle
