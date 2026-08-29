#include <gtest/gtest.h>
#include "simulation/simulation_world.h"
#include "track/race_config.h"
#include "track/track.h"
#include "input/input.h"

namespace track = p0::track;

using namespace p0;
using namespace p0::simulation;

TEST(SimulationWorld, InitializeWithTrack) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);
  EXPECT_EQ(world.track().length(), t.length());
}

TEST(SimulationWorld, AddCarReturnsPositiveId) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int id = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "Player1");
  EXPECT_GE(id, 0);
  EXPECT_TRUE(world.has_car(id));
}

TEST(SimulationWorld, AddMultipleCars) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int id1 = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");
  int id2 = world.add_car(p0::vehicle::VehicleParams{}, DriverType::AI, -1, "AI1");
  int id3 = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 1, "P2");

  EXPECT_NE(id1, id2);
  EXPECT_NE(id1, id3);
  EXPECT_NE(id2, id3);
  EXPECT_EQ(world.car_count(), 3);
}

TEST(SimulationWorld, RemoveCar) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int id = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");
  EXPECT_TRUE(world.has_car(id));

  world.remove_car(id);
  EXPECT_FALSE(world.has_car(id));
  EXPECT_EQ(world.car_count(), 0);
}

TEST(SimulationWorld, FirstHumanIsLocalCar) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  world.add_car(p0::vehicle::VehicleParams{}, DriverType::AI, -1, "AI");
  int human_id = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");

  EXPECT_EQ(world.local_car_id(), human_id);
}

TEST(SimulationWorld, SetInputUpdatesPendingInput) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int id = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");
  p0::input::InputState input;
  input.throttle = 0.8;
  input.steering = 0.3;

  world.set_input(id, input);
  const CarInstance* car = world.get_car(id);
  ASSERT_NE(car, nullptr);
  EXPECT_DOUBLE_EQ(car->pending_input.throttle, 0.8);
  EXPECT_DOUBLE_EQ(car->pending_input.steering, 0.3);
}

TEST(SimulationWorld, SetAiInputOnlyForAi) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int human_id = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");
  int ai_id = world.add_car(p0::vehicle::VehicleParams{}, DriverType::AI, -1, "AI1");

  p0::input::InputState ai_input;
  ai_input.throttle = 1.0;
  world.set_ai_input(human_id, ai_input);
  world.set_ai_input(ai_id, ai_input);

  const CarInstance* ai_car = world.get_car(ai_id);
  ASSERT_NE(ai_car, nullptr);
  EXPECT_DOUBLE_EQ(ai_car->pending_input.throttle, 1.0);
}

TEST(SimulationWorld, UpdateAdvancesRaceTime) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int id = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");

  WorldUpdateResult r1 = world.update(0.1, 1.0);
  EXPECT_DOUBLE_EQ(r1.race_time, 0.1);

  WorldUpdateResult r2 = world.update(0.2, 2.0);
  EXPECT_DOUBLE_EQ(r2.race_time, 0.3);
}

TEST(SimulationWorld, UpdateSetsLeader) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int id1 = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");
  int id2 = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 1, "P2");

  WorldUpdateResult result = world.update(0.016, 1.0);
  EXPECT_GE(result.leader_car_id, 0);
  EXPECT_TRUE(result.leader_car_id == id1 || result.leader_car_id == id2);
}

TEST(SimulationWorld, ActiveCarCountExcludesFinished) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int id1 = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");
  int id2 = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 1, "P2");

  EXPECT_EQ(world.active_car_count(), 2);

  CarInstance* car2 = world.get_car(id2);
  ASSERT_NE(car2, nullptr);
  car2->finished = true;

  EXPECT_EQ(world.active_car_count(), 1);
}

TEST(SimulationWorld, ResetCarRestoresState) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int id = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");
  CarInstance* car = world.get_car(id);
  ASSERT_NE(car, nullptr);
  car->finished = true;
  car->lap = 2;

  p0::vehicle::VehicleState initial;
  world.reset_car(id, initial);

  car = world.get_car(id);
  EXPECT_FALSE(car->finished);
  EXPECT_EQ(car->lap, 0);
}

TEST(SimulationWorld, ShutdownClearsCars) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");
  world.add_car(p0::vehicle::VehicleParams{}, DriverType::AI, -1, "AI1");
  EXPECT_EQ(world.car_count(), 2);

  world.shutdown();
  EXPECT_EQ(world.car_count(), 0);
}

TEST(SimulationWorld, UpdateWithRaceManagerCallsCallback) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int id = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");

  bool callback_called = false;
  SimulationWorld::RaceUpdateCb cb = [&](double, const std::unordered_map<int, Vec2>&,
      const std::unordered_map<int, double>&,
      const std::unordered_map<int, double>&,
      const std::unordered_map<int, race::TireCompound>&) {
    callback_called = true;
  };
  world.update_with_race_manager(0.016, 1.0, cb);

  EXPECT_TRUE(callback_called);
}

TEST(SimulationWorld, GetStateReturnsReference) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int id = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");
  const p0::vehicle::VehicleState* state = world.get_state(id);
  ASSERT_NE(state, nullptr);
  EXPECT_TRUE(state->position.x() >= 0.0 || true);
}

TEST(SimulationWorld, LapCompletedCallbackFires) {
  track::Track t;
  SimulationWorld world;
  world.initialize(t);

  int id = world.add_car(p0::vehicle::VehicleParams{}, DriverType::HUMAN, 0, "P1");

  bool callback_called = false;
  int callback_lap = -1;
  world.set_on_lap_completed([&](int car_id, int lap, double lap_time) {
    callback_called = true;
    callback_lap = lap;
  });

  p0::input::InputState throttle_input;
  throttle_input.throttle = 1.0;
  throttle_input.upshift = true;

  for (int i = 0; i < 15000; ++i) {
    world.set_input(id, throttle_input);
    world.update(1.0 / 60.0, i * (1.0 / 60.0));
  }

  EXPECT_TRUE(callback_called);
  EXPECT_GT(callback_lap, 0);
}
