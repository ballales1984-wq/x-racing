// Project 0 — unit tests for vehicle, mesh generation and export
// Verifies VehicleState defaults, geometry derivation and mesh I/O.
#include <gtest/gtest.h>
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_generator.h"
#include "vehicle/mesh_exporter.h"
#include "assets/mesh.h"

using namespace p0;

// VehicleState should initialize with sensible defaults
TEST(VehicleStateV2, Initialization) {
  vehicle::VehicleState state;
  EXPECT_DOUBLE_EQ(state.position.x(), 0.0);
  EXPECT_DOUBLE_EQ(state.position.y(), 0.0);
  EXPECT_DOUBLE_EQ(state.speed, 0.0);
  EXPECT_EQ(state.gear, 1);
  EXPECT_EQ(state.lap, 0);
}

// VehicleState tire temps should start at 300 K
TEST(VehicleStateV2, DefaultTireTemps) {
  vehicle::VehicleState state;
  EXPECT_DOUBLE_EQ(state.front_tire_temp, 300.0);
  EXPECT_DOUBLE_EQ(state.rear_tire_temp, 300.0);
}

// VehicleState tire wear should start at 1.0 (new)
TEST(VehicleStateV2, DefaultTireWearNew) {
  vehicle::VehicleState state;
  EXPECT_DOUBLE_EQ(state.front_tire_wear, 1.0);
  EXPECT_DOUBLE_EQ(state.rear_tire_wear, 1.0);
}

// VehicleState weather grip should start at 1.0
TEST(VehicleStateV2, DefaultWeatherGrip) {
  vehicle::VehicleState state;
  EXPECT_DOUBLE_EQ(state.weather_grip_factor, 1.0);
}

// FromParams should derive geometry from vehicle params
TEST(VehicleGenerator, FromParamsDerivesGeometry) {
  vehicle::VehicleParams params;
  vehicle::VehicleGeometry geo = vehicle::VehicleGenerator::FromParams(params);
  EXPECT_GT(geo.body_length, 0.0);
  EXPECT_GT(geo.body_width, 0.0);
  EXPECT_GT(geo.cabin_length, 0.0);
  EXPECT_GT(geo.wheel_radius, 0.0);
}

// Geometry should respect wheelbase
TEST(VehicleGenerator, GeometryRespectsWheelbase) {
  vehicle::VehicleParams params;
  vehicle::VehicleGeometry geo = vehicle::VehicleGenerator::FromParams(params);
  EXPECT_NEAR(geo.wheelbase, params.wheelbase, 1e-9);
  EXPECT_NEAR(geo.track_width, params.track_width, 1e-9);
}

// GenerateCar should produce non-empty mesh
TEST(VehicleGenerator, GenerateCarProducesMesh) {
  vehicle::VehicleParams params;
  vehicle::VehicleGeometry geo = vehicle::VehicleGenerator::FromParams(params);
  vehicle::MeshData mesh = vehicle::VehicleGenerator::GenerateCar(geo);
  EXPECT_GT(mesh.vertices.size(), 0u);
  EXPECT_GT(mesh.indices.size(), 0u);
  EXPECT_EQ(mesh.indices.size() % 3, 0u);
}

// GenerateBody should produce valid indices
TEST(VehicleGenerator, GenerateBodyValidIndices) {
  vehicle::VehicleParams params;
  vehicle::VehicleGeometry geo = vehicle::VehicleGenerator::FromParams(params);
  vehicle::MeshData body = vehicle::VehicleGenerator::GenerateBody(geo);
  for (int idx : body.indices) {
    EXPECT_GE(idx, 0);
    EXPECT_LT(idx, static_cast<int>(body.vertices.size()));
  }
}

// GenerateWheel should produce cylinder-like mesh
TEST(VehicleGenerator, GenerateWheelCylinder) {
  vehicle::VehicleParams params;
  vehicle::VehicleGeometry geo = vehicle::VehicleGenerator::FromParams(params);
  vehicle::MeshData wheel = vehicle::VehicleGenerator::GenerateWheel(geo);
  EXPECT_GT(wheel.vertices.size(), 0u);
  EXPECT_GT(wheel.indices.size(), 0u);
}

// Mesh export/import round-trip should preserve triangle count
TEST(VehicleGenerator, MeshRoundTripPreservesTriangles) {
  vehicle::VehicleParams params;
  vehicle::VehicleGeometry geo = vehicle::VehicleGenerator::FromParams(params);
  vehicle::MeshData original = vehicle::VehicleGenerator::GenerateCar(geo);

  std::string test_file = "D:/x-racing/assets/models/test_roundtrip.obj";
  vehicle::MeshExporter::ExportOBJ(original, test_file);

  p0::assets::Mesh loaded;
  bool ok = p0::assets::MeshLoader::LoadOBJ(test_file, loaded);
  EXPECT_TRUE(ok);
  EXPECT_EQ(loaded.indices.size(), original.indices.size());
}

// Vehicle geometry body dimensions should be positive
TEST(VehicleGenerator, GeometryDimensionsPositive) {
  vehicle::VehicleParams params;
  vehicle::VehicleGeometry geo = vehicle::VehicleGenerator::FromParams(params);
  EXPECT_GT(geo.body_length, 0.0);
  EXPECT_GT(geo.body_width, 0.0);
  EXPECT_GT(geo.body_height, 0.0);
  EXPECT_GT(geo.cabin_length, 0.0);
  EXPECT_GT(geo.cabin_width, 0.0);
  EXPECT_GT(geo.cabin_height, 0.0);
}

// Spoiler width should not exceed body width
TEST(VehicleGenerator, SpoilerWithinBody) {
  vehicle::VehicleParams params;
  vehicle::VehicleGeometry geo = vehicle::VehicleGenerator::FromParams(params);
  EXPECT_LE(geo.spoiler_width, geo.body_width * 1.1);
}
