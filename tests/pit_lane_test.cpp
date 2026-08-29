#include <gtest/gtest.h>
#include "track/pit_lane.h"
#include "track/race_config.h"

using namespace p0::track;

TEST(PitLaneSystem, AssignBoxReturnsValidId) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  def.boxes.push_back(PitBox{1, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);

  int box_id = ps.assign_box(1, 0);
  EXPECT_GE(box_id, 0);
  EXPECT_TRUE(ps.is_box_free(box_id) == false || box_id >= 0);
}

TEST(PitLaneSystem, ReleaseBoxFreesIt) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);

  int box_id = ps.assign_box(1, 0);
  EXPECT_GE(box_id, 0);
  ps.release_box(1);
  EXPECT_TRUE(ps.is_box_free(box_id));
}

TEST(PitLaneSystem, FindBoxForTeam) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  def.boxes.push_back(PitBox{1, 1, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);

  int team_box = ps.find_box_for_team(1);
  EXPECT_EQ(team_box, 1);
}

TEST(PitLaneSystem, FindFreeBox) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);

  int free = ps.find_free_box();
  EXPECT_EQ(free, 0);
}

TEST(PitLaneSystem, SpeedViolationDetected) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  def.speed_zone.speed_limit_m_s = 10.0;
  def.speed_zone.tolerance_m_s = 0.0;
  PitLaneSystem ps(def);

  ps.on_cross_speed_start(1, 0.0, 0.0);
  ps.on_cross_speed_end(1, 1.0, 20.0);

  auto violations = ps.process_speed_violations();
  ASSERT_EQ(violations.size(), 1u);
  EXPECT_EQ(violations[0].car_id, 1);
  EXPECT_DOUBLE_EQ(violations[0].recorded_speed_m_s, 20.0);
  EXPECT_DOUBLE_EQ(violations[0].limit_m_s, 10.0);
}

TEST(PitLaneSystem, NoViolationUnderLimit) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  def.speed_zone.speed_limit_m_s = 20.0;
  PitLaneSystem ps(def);

  ps.on_cross_speed_start(1, 0.0, 0.0);
  ps.on_cross_speed_end(1, 1.0, 10.0);

  auto violations = ps.process_speed_violations();
  EXPECT_TRUE(violations.empty());
}

TEST(PitLaneSystem, NoViolationWithoutEntry) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  def.speed_zone.speed_limit_m_s = 10.0;
  PitLaneSystem ps(def);

  ps.on_cross_speed_end(1, 1.0, 20.0);
  auto violations = ps.process_speed_violations();
  EXPECT_TRUE(violations.empty());
}

TEST(PitLaneSystem, CalculateServiceRefuelOnly) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);

  CarFuelState fuel;
  fuel.current_fuel_l = 50.0;
  fuel.fuel_capacity_l = 100.0;
  CarTireState tires;
  PitServiceResult r = ps.calculate_service(fuel, tires, p0::race::TireCompound::MEDIUM,
                                                  true, false, false, 0.0, 2.0, 3.0, 5.0);
  EXPECT_TRUE(r.refueled);
  EXPECT_GT(r.fuel_added_l, 0.0);
  EXPECT_GT(r.refuel_time_s, 0.0);
  EXPECT_FALSE(r.tires_changed);
  EXPECT_FALSE(r.repaired);
}

TEST(PitLaneSystem, CalculateServiceTireChangeOnly) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);

  CarFuelState fuel;
  CarTireState tires;
  tires.current_compound = p0::race::TireCompound::SOFT;
  PitServiceResult r = ps.calculate_service(fuel, tires, p0::race::TireCompound::HARD,
                                                  false, true, false, 0.0, 2.0, 3.0, 5.0);
  EXPECT_TRUE(r.tires_changed);
  EXPECT_EQ(r.new_compound, p0::race::TireCompound::HARD);
  EXPECT_DOUBLE_EQ(r.tire_change_time_s, 3.0);
  EXPECT_FALSE(r.refueled);
}

TEST(PitLaneSystem, CalculateServiceRepairOnly) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);

  CarFuelState fuel;
  CarTireState tires;
  PitServiceResult r = ps.calculate_service(fuel, tires, p0::race::TireCompound::MEDIUM,
                                                  false, false, true, 50.0, 2.0, 3.0, 5.0);
  EXPECT_TRUE(r.repaired);
  EXPECT_DOUBLE_EQ(r.repair_time_s, 10.0);
  EXPECT_FALSE(r.refueled);
  EXPECT_FALSE(r.tires_changed);
}

TEST(PitLaneSystem, CalculateServiceTotalTimeIsMax) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);

  CarFuelState fuel;
  fuel.current_fuel_l = 10.0;
  fuel.fuel_capacity_l = 100.0;
  CarTireState tires;
  tires.current_compound = p0::race::TireCompound::SOFT;
  PitServiceResult r = ps.calculate_service(fuel, tires, p0::race::TireCompound::HARD,
                                                  true, true, true, 20.0, 2.0, 3.0, 5.0);
  EXPECT_DOUBLE_EQ(r.total_service_time_s, std::max(r.refuel_time_s, std::max(r.tire_change_time_s, r.repair_time_s)));
}

TEST(PitLaneSystem, CarsInPitLaneCounts) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);

  ps.assign_box(1, 0);
  ps.car_state(1).state = p0::race::PitStopState::ENTERING_PIT_LANE;
  ps.car_state(2).state = p0::race::PitStopState::PIT_EXIT_NAVIGATION;

  EXPECT_EQ(ps.cars_in_pit_lane(), 2);
}

TEST(PitLaneSystem, CarsStoppedCounts) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);

  ps.assign_box(1, 0);
  ps.car_state(1).state = p0::race::PitStopState::STOPPED_AT_BOX;
  ps.car_state(2).state = p0::race::PitStopState::SERVICING;
  ps.car_state(3).state = p0::race::PitStopState::PIT_LANE_NAVIGATION;

  EXPECT_EQ(ps.cars_stopped(), 2);
}

TEST(PitLaneSystem, ResetCarClearsState) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);

  ps.assign_box(1, 0);
  ps.reset_car(1);
  EXPECT_EQ(ps.car_state(1).assigned_box_id, -1);
}
