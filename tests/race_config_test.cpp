#include <gtest/gtest.h>
#include "track/race_config.h"

using namespace p0::race;

TEST(TireCompoundName, Soft) {
  EXPECT_STREQ(tire_compound_name(TireCompound::SOFT), "Soft");
}

TEST(TireCompoundName, Medium) {
  EXPECT_STREQ(tire_compound_name(TireCompound::MEDIUM), "Medium");
}

TEST(TireCompoundName, Hard) {
  EXPECT_STREQ(tire_compound_name(TireCompound::HARD), "Hard");
}

TEST(TireCompoundName, Wet) {
  EXPECT_STREQ(tire_compound_name(TireCompound::WET), "Wet");
}

TEST(TireCompoundName, Intermediate) {
  EXPECT_STREQ(tire_compound_name(TireCompound::INTERMEDIATE), "Intermediate");
}

TEST(TireCompoundName, UnknownFallback) {
  EXPECT_STREQ(tire_compound_name(static_cast<TireCompound>(255)), "Unknown");
}

TEST(RaceDefinition, PitRequired) {
  RaceDefinition def;
  def.pit_min_stops = 1;
  EXPECT_TRUE(def.is_pit_required());
}

TEST(RaceDefinition, NoPitRequired) {
  RaceDefinition def;
  def.pit_min_stops = 0;
  EXPECT_FALSE(def.is_pit_required());
}

TEST(RaceDefinition, UsesLapsWhenDistanceZero) {
  RaceDefinition def;
  def.race_distance_m = 0.0;
  def.laps = 15;
  EXPECT_TRUE(def.uses_laps());
}

TEST(RaceDefinition, UsesDistanceWhenPositive) {
  RaceDefinition def;
  def.race_distance_m = 5000.0;
  def.laps = 0;
  EXPECT_FALSE(def.uses_laps());
}

TEST(Enums, RaceTypeValues) {
  EXPECT_EQ(static_cast<uint8_t>(RaceType::SPRINT), 0);
  EXPECT_EQ(static_cast<uint8_t>(RaceType::ENDURANCE), 1);
  EXPECT_EQ(static_cast<uint8_t>(RaceType::CUSTOM), 2);
}

TEST(Enums, PitStopStateValues) {
  EXPECT_EQ(static_cast<uint8_t>(PitStopState::NONE), 0);
  EXPECT_EQ(static_cast<uint8_t>(PitStopState::REQUESTED), 1);
  EXPECT_EQ(static_cast<uint8_t>(PitStopState::COMPLETE), 13);
  EXPECT_EQ(static_cast<uint8_t>(PitStopState::ABANDONED), 14);
}

TEST(Enums, RaceSessionStateValues) {
  EXPECT_EQ(static_cast<uint8_t>(RaceSessionState::PREGAME), 0);
  EXPECT_EQ(static_cast<uint8_t>(RaceSessionState::FORMATION), 1);
  EXPECT_EQ(static_cast<uint8_t>(RaceSessionState::GREEN_FLAG_RUNNING), 4);
  EXPECT_EQ(static_cast<uint8_t>(RaceSessionState::CHECKERED_FLAG), 6);
}

TEST(ServiceFlags, BitFieldDefaults) {
  ServiceFlags flags;
  EXPECT_FALSE(flags.refueling);
  EXPECT_FALSE(flags.tire_change);
  EXPECT_FALSE(flags.repair);
}

TEST(ServiceFlags, BitFieldAssignment) {
  ServiceFlags flags;
  flags.refueling = true;
  flags.tire_change = true;
  EXPECT_TRUE(flags.refueling);
  EXPECT_TRUE(flags.tire_change);
  EXPECT_FALSE(flags.repair);
}

TEST(CarFuelState, Defaults) {
  CarFuelState fuel;
  EXPECT_DOUBLE_EQ(fuel.current_fuel_l, 0.0);
  EXPECT_DOUBLE_EQ(fuel.fuel_capacity_l, 0.0);
  EXPECT_DOUBLE_EQ(fuel.consumption_per_lap_l, 0.0);
  EXPECT_DOUBLE_EQ(fuel.consumption_per_m_l, 0.0);
}

TEST(CarTireState, Defaults) {
  CarTireState tires;
  EXPECT_EQ(tires.current_compound, TireCompound::MEDIUM);
  EXPECT_EQ(tires.current_stint_laps, 0);
  EXPECT_DOUBLE_EQ(tires.wear_percent, 0.0);
}

TEST(CarAssignment, Defaults) {
  CarAssignment a;
  EXPECT_EQ(a.car_id, 0);
  EXPECT_EQ(a.team_id, 0);
  EXPECT_EQ(a.grid_slot, 0);
  EXPECT_EQ(a.pit_box_id, -1);
  EXPECT_EQ(a.start_tire, TireCompound::MEDIUM);
}
