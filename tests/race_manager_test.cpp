#include <gtest/gtest.h>
#include "track/race_manager.h"
#include "track/track_data.h"

using namespace p0::track;

// Helper: create a minimally valid TrackData that passes all validation checks
static TrackData make_valid_track() {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;
  track.direction = "CLOCKWISE";

  for (int i = 0; i < 4; ++i) {
    track.waypoints.push_back(Waypoint{i, make_transform(p0::Vec2(i * 250.0, 0.0), p0::Vec2(1.0, 0.0)), 15.0, 3.0});
  }

  track.racing_line.push_back(RacingLineSample{});

  for (int i = 0; i < 20; ++i) {
    track.grid.slots.push_back(GridSlot{i, make_transform(p0::Vec2(0.0, i * 8.0), p0::Vec2(1.0, 0.0)), 4.0, 10.0});
  }

  track.pit_lane.speed_zone.speed_limit_m_s = 16.67;
  track.pit_lane.path.push_back(PitLanePathPoint{});
  PitBox box{0, 0, {}, {}, {}, {}, 4.0, 6.0};
  box.position.position = p0::Vec2(5.0, 0.0);
  track.pit_lane.boxes.push_back(box);

  return track;
}

static p0::race::RaceDefinition make_valid_race() {
  p0::race::RaceDefinition race;
  race.laps = 3;
  race.track_id = "test";
  return race;
}

TEST(RaceManager, InitializesWithAssignments) {
  TrackData track = make_valid_track();

  p0::race::RaceDefinition race = make_valid_race();

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  a.pit_box_id = 0;
  assignments.push_back(a);

  std::vector<p0::race::TeamDefinition> teams;
  RaceManager mgr(track, race);

  bool ok = mgr.initialize(assignments, teams);
  EXPECT_TRUE(ok);
  EXPECT_TRUE(mgr.is_valid());
}

TEST(RaceManager, EmptyAssignmentsFailsInit) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 1000.0;

  p0::race::RaceDefinition race;
  std::vector<p0::race::CarAssignment> assignments;
  std::vector<p0::race::TeamDefinition> teams;
  RaceManager mgr(track, race);

  bool ok = mgr.initialize(assignments, teams);
  EXPECT_FALSE(ok);
}

TEST(RaceManager, StartRaceSetsFormation) {
  TrackData track = make_valid_track();
  p0::race::RaceDefinition race = make_valid_race();

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();

  EXPECT_EQ(mgr.session_state(), p0::race::RaceSessionState::FORMATION);
}

TEST(RaceManager, SessionStateAdvancesToGreenFlag) {
  TrackData track = make_valid_track();
  p0::race::RaceDefinition race = make_valid_race();
  race.formation_lap = 1;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();
  mgr.update(200.0, {}, {}, {}, {}, {});
  mgr.update(200.0, {}, {}, {}, {}, {});

  EXPECT_EQ(mgr.session_state(), p0::race::RaceSessionState::GREEN_FLAG_RUNNING);
  EXPECT_EQ(mgr.current_lap(), 1);
}

TEST(RaceManager, CheckeredFlagWhenLapsComplete) {
  TrackData track = make_valid_track();
  track.checkpoints.push_back(Checkpoint{0, {}, 20.0, false, 0});

  p0::race::RaceDefinition race = make_valid_race();
  race.laps = 2;
  race.formation_lap = 0;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  // Provide car_distances so lap counter can detect wrap-around
  std::unordered_map<int, double> distances;

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();

  // Lap 1 complete: position wraps from 900 to 100
  distances[1] = 100.0;
  mgr.update(1.0, {}, {}, distances, {}, {});
  distances[1] = 900.0;
  mgr.update(2.0, {}, {}, distances, {}, {});
  // Lap 2 complete: wraps from 900 back to 100
  distances[1] = 100.0;
  mgr.update(3.0, {}, {}, distances, {}, {});
  distances[1] = 900.0;
  mgr.update(4.0, {}, {}, distances, {}, {});
  // Lap 2 complete: wraps again
  distances[1] = 100.0;
  mgr.update(5.0, {}, {}, distances, {}, {});

  EXPECT_EQ(mgr.session_state(), p0::race::RaceSessionState::CHECKERED_FLAG);
}

TEST(RaceManager, LapCounterDetectsLapCompletion) {
  TrackData track = make_valid_track();
  track.checkpoints.push_back(Checkpoint{0, {}, 20.0, false, 0});

  p0::race::RaceDefinition race = make_valid_race();

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  std::unordered_map<int, double> distances;

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();

  distances[1] = 800.0;
  mgr.update(1.0, {}, {}, distances, {}, {});
  EXPECT_EQ(mgr.current_lap(), 0);

  distances[1] = 100.0;
  mgr.update(2.0, {}, {}, distances, {}, {});
  EXPECT_EQ(mgr.current_lap(), 1);
}

TEST(RaceManager, RequestPitStopOnlyWhenRunning) {
  TrackData track = make_valid_track();

  p0::race::RaceDefinition race = make_valid_race();

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  a.pit_box_id = 0;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});

  bool ok = mgr.request_pit_stop(1, p0::race::TireCompound::SOFT, true, true, false);
  EXPECT_FALSE(ok);
}

TEST(RaceManager, RequestPitStopWhenRunningSucceeds) {
  TrackData track = make_valid_track();
  p0::race::RaceDefinition race = make_valid_race();
  race.grid_slots = 1;
  race.max_cars = 1;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  a.pit_box_id = 0;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();
  mgr.update(200.0, {}, {}, {}, {}, {});
  mgr.update(200.0, {}, {}, {}, {}, {});

  bool ok = mgr.request_pit_stop(1, p0::race::TireCompound::SOFT, true, true, false);
  EXPECT_TRUE(ok);
  EXPECT_TRUE(mgr.pit_manager().is_pit_stop_active(1));
}

TEST(RaceManager, ValidationIssuesPropagated) {
  TrackData track;
  track.track_id = "test";
  track.length_m = 50.0;

  p0::race::RaceDefinition race;
  race.laps = 0;
  race.race_distance_m = 0.0;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  a.pit_box_id = 0;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});

  EXPECT_FALSE(mgr.validation_issues().empty());
}

TEST(RaceManager, DebugReportContainsSessionInfo) {
  TrackData track = make_valid_track();
  p0::race::RaceDefinition race = make_valid_race();

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  a.pit_box_id = 0;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();

  std::string report = mgr.debug_report();
  EXPECT_NE(report.find("Session:"), std::string::npos);
  EXPECT_NE(report.find("Lap:"), std::string::npos);
}
