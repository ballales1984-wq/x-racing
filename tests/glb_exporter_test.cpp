// Project 0 — unit tests for the GLB binary exporter
// Verifies GLB header validity, chunk layout and error handling.
#include <gtest/gtest.h>
#include "vehicle/vehicle_generator.h"
#include "vehicle/glb_exporter.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

using namespace p0::vehicle;

namespace {
// Read the whole file as bytes.
std::vector<uint8_t> ReadAllBytes(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());
}

// Read a little-endian uint32 from a byte buffer at offset.
uint32_t ReadU32(const std::vector<uint8_t>& b, size_t off) {
  return static_cast<uint32_t>(b[off]) |
         (static_cast<uint32_t>(b[off + 1]) << 8) |
         (static_cast<uint32_t>(b[off + 2]) << 16) |
         (static_cast<uint32_t>(b[off + 3]) << 24);
}
}  // namespace

// Exporting a generated car mesh should produce a valid GLB file.
TEST(GLBExporter, ExportCarProducesValidGLB) {
  VehicleGeometry geo = VehicleGenerator::FromParams(VehicleParams());
  const std::string path = "D:/x-racing/assets/models/test_export.glb";

  ASSERT_TRUE(GLBExporter::ExportCarGLB(geo, path));

  auto bytes = ReadAllBytes(path);
  ASSERT_GE(bytes.size(), 12u);

  // 12-byte GLB header: magic "glTF", version 2, total length.
  EXPECT_EQ(ReadU32(bytes, 0), 0x46546C67u);  // "glTF"
  EXPECT_EQ(ReadU32(bytes, 4), 2u);           // version 2
  EXPECT_EQ(ReadU32(bytes, 8), static_cast<uint32_t>(bytes.size()));

  // JSON chunk header.
  const uint32_t jsonLen = ReadU32(bytes, 12);
  EXPECT_EQ(ReadU32(bytes, 16), 0x4E4F534Au);  // "JSON"
  ASSERT_GE(bytes.size(), 12u + 8u + jsonLen);

  // JSON should describe a glTF 2.0 asset with the expected accessors.
  std::string json(reinterpret_cast<const char*>(bytes.data() + 20), jsonLen);
  EXPECT_NE(json.find("\"version\": \"2.0\""), std::string::npos);
  EXPECT_NE(json.find("\"POSITION\""), std::string::npos);
  EXPECT_NE(json.find("\"indices\""), std::string::npos);

  // Binary chunk header.
  const size_t binOff = 12u + 8u + jsonLen;
  ASSERT_GE(bytes.size(), binOff + 8u);
  const uint32_t binLen = ReadU32(bytes, binOff);
  EXPECT_EQ(ReadU32(bytes, binOff + 4), 0x004E4942u);  // "BIN\0"
  EXPECT_EQ(binOff + 8u + binLen, bytes.size());
}

// The binary buffer must be large enough to hold positions, normals, indices, UVs.
TEST(GLBExporter, BinaryBufferSizedForAllAttributes) {
  MeshData mesh = VehicleGenerator::GenerateCar(VehicleGenerator::FromParams(VehicleParams()));
  const std::string path = "D:/x-racing/assets/models/test_export2.glb";

  ASSERT_TRUE(GLBExporter::ExportGLB(mesh, path));
  auto bytes = ReadAllBytes(path);

  const uint32_t jsonLen = ReadU32(bytes, 12);
  const size_t binOff = 12u + 8u + jsonLen;
  const uint32_t binLen = ReadU32(bytes, binOff);

  const uint64_t expected = static_cast<uint64_t>(mesh.vertices.size()) * 12u +
                            static_cast<uint64_t>(mesh.normals.size()) * 12u +
                            static_cast<uint64_t>(mesh.indices.size()) * 4u +
                            static_cast<uint64_t>(mesh.uvs.size()) * 8u;
  // Binary chunk may be padded to a 4-byte boundary.
  EXPECT_GE(binLen, expected);
  EXPECT_LE(binLen - expected, 3u);
}

// Exporting to an invalid (nonexistent) directory should fail gracefully.
TEST(GLBExporter, ExportToInvalidPathReturnsFalse) {
  MeshData mesh = VehicleGenerator::GenerateCar(VehicleGenerator::FromParams(VehicleParams()));
  const std::string path = "D:/x-racing/__no_such_dir__/vehicle.glb";
  EXPECT_FALSE(GLBExporter::ExportGLB(mesh, path));
}
