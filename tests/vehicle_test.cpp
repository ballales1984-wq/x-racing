// Project 0 — unit tests for vehicle, mesh generation and export
// Verifies VehicleState defaults, geometry derivation and mesh I/O.
#include <gtest/gtest.h>
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_generator.h"
#include "vehicle/car_model.h"
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
  std::vector<p0::assets::Material> loaded_materials;
  bool ok = p0::assets::MeshLoader::LoadOBJ(test_file, loaded, loaded_materials);
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

// CarRegistry should be a singleton with two default models registered
TEST(CarRegistry, HasTwoDefaultModels) {
  auto& registry = vehicle::CarRegistry::instance();
  EXPECT_EQ(registry.size(), 2u);
}

// CarRegistry should find the Porsche 911 model by id
TEST(CarRegistry, FindsPorsche911) {
  auto& registry = vehicle::CarRegistry::instance();
  const auto* model = registry.get("porsche_911");
  ASSERT_NE(model, nullptr);
  EXPECT_EQ(model->name, "Porsche 911 Turbo S");
  EXPECT_GT(model->params.mass, 0.0);
  EXPECT_GT(model->params.max_power, 0.0);
  EXPECT_GT(model->geometry.body_length, 0.0);
}

// CarRegistry should find the Ferrari F12 model by id
TEST(CarRegistry, FindsFerrariF12) {
  auto& registry = vehicle::CarRegistry::instance();
  const auto* model = registry.get("ferrari_f12");
  ASSERT_NE(model, nullptr);
  EXPECT_EQ(model->name, "Ferrari F12berlinetta");
  EXPECT_GT(model->params.mass, 0.0);
  EXPECT_GT(model->params.max_power, 0.0);
  EXPECT_GT(model->geometry.body_length, 0.0);
}

// CarRegistry should return nullptr for unknown id
TEST(CarRegistry, UnknownIdReturnsNullptr) {
  auto& registry = vehicle::CarRegistry::instance();
  EXPECT_EQ(registry.get("nonexistent"), nullptr);
  EXPECT_FALSE(registry.has("nonexistent"));
}

// CarRegistry should produce different params for different cars
TEST(CarRegistry, CarsHaveDistinctParams) {
  auto& registry = vehicle::CarRegistry::instance();
  const auto* porsche = registry.get("porsche_911");
  const auto* ferrari = registry.get("ferrari_f12");
  ASSERT_NE(porsche, nullptr);
  ASSERT_NE(ferrari, nullptr);

  EXPECT_NE(porsche->params.mass, ferrari->params.mass);
  EXPECT_NE(porsche->params.max_power, ferrari->params.max_power);
  EXPECT_NE(porsche->params.wheelbase, ferrari->params.wheelbase);
  EXPECT_NE(porsche->params.camber_gain_per_roll, ferrari->params.camber_gain_per_roll);
}

// Each car model should generate a valid, non-empty mesh
TEST(CarRegistry, GenerateMeshForPorsche) {
  auto& registry = vehicle::CarRegistry::instance();
  const auto* model = registry.get("porsche_911");
  ASSERT_NE(model, nullptr);

  vehicle::VehicleGeometry geo = vehicle::VehicleGenerator::FromParams(model->params);
  vehicle::MeshData mesh = vehicle::VehicleGenerator::GenerateCar(geo);
  EXPECT_GT(mesh.vertices.size(), 0u);
  EXPECT_GT(mesh.indices.size(), 0u);
  EXPECT_EQ(mesh.indices.size() % 3, 0u);
}

// All models in the registry should have valid geometry assigned
TEST(CarRegistry, AllModelsHaveGeometry) {
  auto& registry = vehicle::CarRegistry::instance();
  for (const auto& model : registry.all()) {
    EXPECT_GT(model.geometry.body_length, 0.0);
    EXPECT_GT(model.geometry.body_width, 0.0);
    EXPECT_GT(model.geometry.body_height, 0.0);
    EXPECT_GT(model.geometry.wheel_radius, 0.0);
  }
}
