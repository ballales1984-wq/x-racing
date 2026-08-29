#include <gtest/gtest.h>
#include "track/validation.h"
#include "track/track_data.h"

using namespace p0::track;

TEST(ValidationEngine, GeometryTooFewWaypoints) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;
  track.waypoints.push_back(Waypoint{0, {}, 15.0, 3.0});
  track.waypoints.push_back(Waypoint{1, {}, 15.0, 3.0});
  track.waypoints.push_back(Waypoint{2, {}, 15.0, 3.0});

  p0::race::RaceDefinition race;
  std::vector<p0::race::CarAssignment> assignments;
  ValidationEngine engine(track, race, assignments);

  auto issues = engine.validate_geometry();
  bool found_geo001 = false;
  for (const auto& issue : issues) {
    if (issue.code == "GEO_001") found_geo001 = true;
  }
  EXPECT_TRUE(found_geo001);
}

TEST(ValidationEngine, GeometryTooShort) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 50.0;
  track.waypoints.push_back(Waypoint{0, {}, 15.0, 3.0});
  track.waypoints.push_back(Waypoint{1, {}, 15.0, 3.0});
  track.waypoints.push_back(Waypoint{2, {}, 15.0, 3.0});
  track.waypoints.push_back(Waypoint{3, {}, 15.0, 3.0});

  p0::race::RaceDefinition race;
  std::vector<p0::race::CarAssignment> assignments;
  ValidationEngine engine(track, race, assignments);

  auto issues = engine.validate_geometry();
  bool found_geo002 = false;
  for (const auto& issue : issues) {
    if (issue.code == "GEO_002") found_geo002 = true;
  }
  EXPECT_TRUE(found_geo002);
}

TEST(ValidationEngine, GeometryEmptyRacingLine) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;
  track.waypoints.push_back(Waypoint{0, {}, 15.0, 3.0});
  track.waypoints.push_back(Waypoint{1, {}, 15.0, 3.0});
  track.waypoints.push_back(Waypoint{2, {}, 15.0, 3.0});
  track.waypoints.push_back(Waypoint{3, {}, 15.0, 3.0});

  p0::race::RaceDefinition race;
  std::vector<p0::race::CarAssignment> assignments;
  ValidationEngine engine(track, race, assignments);

  auto issues = engine.validate_geometry();
  bool found_geo003 = false;
  for (const auto& issue : issues) {
    if (issue.code == "GEO_003") found_geo003 = true;
  }
  EXPECT_TRUE(found_geo003);
}

TEST(ValidationEngine, ValidGeometryPasses) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 2000.0;
  track.waypoints.push_back(Waypoint{0, {}, 15.0, 3.0});
  track.waypoints.push_back(Waypoint{1, {}, 15.0, 3.0});
  track.waypoints.push_back(Waypoint{2, {}, 15.0, 3.0});
  track.waypoints.push_back(Waypoint{3, {}, 15.0, 3.0});
  track.racing_line.push_back(RacingLineSample{});
  track.start_finish.transform.position = p0::Vec2(0.0, 0.0);
  track.start_finish.transform.forward = p0::Vec2(1.0, 0.0);

  p0::race::RaceDefinition race;
  std::vector<p0::race::CarAssignment> assignments;
  ValidationEngine engine(track, race, assignments);

  auto issues = engine.validate_geometry();
  bool has_error = false;
  for (const auto& issue : issues) {
    if (issue.severity == p0::race::ValidationSeverity::ERROR) has_error = true;
  }
  EXPECT_FALSE(has_error);
}

TEST(ValidationEngine, InvalidDirection) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;
  track.direction = "INVALID";

  p0::race::RaceDefinition race;
  std::vector<p0::race::CarAssignment> assignments;
  ValidationEngine engine(track, race, assignments);

  auto issues = engine.validate_direction();
  bool found_dir001 = false;
  for (const auto& issue : issues) {
    if (issue.code == "DIR_001") found_dir001 = true;
  }
  EXPECT_TRUE(found_dir001);
}

TEST(ValidationEngine, GridSlotOverlap) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;
  track.grid.slots.push_back(GridSlot{0, {p0::Vec2(0.0, 0.0), {1.0, 0.0}}, 4.0, 10.0});
  track.grid.slots.push_back(GridSlot{1, {p0::Vec2(1.0, 0.0), {1.0, 0.0}}, 4.0, 10.0});

  p0::race::RaceDefinition race;
  race.max_cars = 2;
  std::vector<p0::race::CarAssignment> assignments;
  ValidationEngine engine(track, race, assignments);

  auto issues = engine.validate_grid();
  bool found_grid003 = false;
  for (const auto& issue : issues) {
    if (issue.code == "GRID_003") found_grid003 = true;
  }
  EXPECT_TRUE(found_grid003);
}

TEST(ValidationEngine, PitLaneValidationMissingPath) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;
  track.pit_lane.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});

  p0::race::RaceDefinition race;
  std::vector<p0::race::CarAssignment> assignments;
  ValidationEngine engine(track, race, assignments);

  auto issues = engine.validate_pit_lane();
  bool found_pit001 = false;
  for (const auto& issue : issues) {
    if (issue.code == "PIT_001") found_pit001 = true;
  }
  EXPECT_TRUE(found_pit001);
}

TEST(ValidationEngine, RaceMinStopsExceedsLaps) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;

  p0::race::RaceDefinition race;
  race.laps = 3;
  race.pit_min_stops = 5;

  std::vector<p0::race::CarAssignment> assignments;
  ValidationEngine engine(track, race, assignments);

  auto issues = engine.validate_race();
  bool found_race003 = false;
  for (const auto& issue : issues) {
    if (issue.code == "RACE_003") found_race003 = true;
  }
  EXPECT_TRUE(found_race003);
}

TEST(ValidationEngine, DuplicateGridSlotAssignment) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;

  p0::race::RaceDefinition race;
  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a1;
  a1.car_id = 1;
  a1.grid_slot = 5;
  p0::race::CarAssignment a2;
  a2.car_id = 2;
  a2.grid_slot = 5;
  assignments.push_back(a1);
  assignments.push_back(a2);

  ValidationEngine engine(track, race, assignments);
  auto issues = engine.validate_assignments();
  bool found_asgn001 = false;
  for (const auto& issue : issues) {
    if (issue.code == "ASGN_001") found_asgn001 = true;
  }
  EXPECT_TRUE(found_asgn001);
}

TEST(ValidationEngine, StartFuelExceedsCapacity) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;

  p0::race::RaceDefinition race;
  race.fuel_capacity_l = 100.0;
  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  a.start_fuel_l = 150.0;
  assignments.push_back(a);

  ValidationEngine engine(track, race, assignments);
  auto issues = engine.validate_assignments();
  bool found_asgn003 = false;
  for (const auto& issue : issues) {
    if (issue.code == "ASGN_003") found_asgn003 = true;
  }
  EXPECT_TRUE(found_asgn003);
}

TEST(ValidationEngine, SeverityNameMapping) {
  EXPECT_STREQ(ValidationEngine::severity_name(p0::race::ValidationSeverity::ERROR).c_str(), "ERROR");
  EXPECT_STREQ(ValidationEngine::severity_name(p0::race::ValidationSeverity::WARNING).c_str(), "WARNING");
  EXPECT_STREQ(ValidationEngine::severity_name(p0::race::ValidationSeverity::INFO).c_str(), "INFO");
}

TEST(ValidationEngine, DuplicateCheckpointIds) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;
  track.checkpoints.push_back(Checkpoint{0, {}, 20.0, false, 0});
  track.checkpoints.push_back(Checkpoint{1, {}, 20.0, false, 0});
  track.checkpoints.push_back(Checkpoint{0, {}, 20.0, false, 0});

  p0::race::RaceDefinition race;
  std::vector<p0::race::CarAssignment> assignments;
  ValidationEngine engine(track, race, assignments);

  auto issues = engine.validate_checkpoints();
  bool found_chk002 = false;
  for (const auto& issue : issues) {
    if (issue.code == "CHK_002") found_chk002 = true;
  }
  EXPECT_TRUE(found_chk002);
}

TEST(ValidationEngine, TooFewCheckpoints) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;
  track.checkpoints.push_back(Checkpoint{0, {}, 20.0, false, 0});

  p0::race::RaceDefinition race;
  std::vector<p0::race::CarAssignment> assignments;
  ValidationEngine engine(track, race, assignments);

  auto issues = engine.validate_checkpoints();
  bool found_chk001 = false;
  for (const auto& issue : issues) {
    if (issue.code == "CHK_001") found_chk001 = true;
  }
  EXPECT_TRUE(found_chk001);
}
