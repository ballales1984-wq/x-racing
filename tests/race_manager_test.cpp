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

TEST(RaceManager, StartRaceEntersGrid) {
  TrackData track = make_valid_track();
  p0::race::RaceDefinition race = make_valid_race();

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();

  EXPECT_EQ(mgr.session_state(), p0::race::RaceSessionState::GRID);
}

TEST(RaceManager, CountdownTransitionsToFormation) {
  TrackData track = make_valid_track();
  p0::race::RaceDefinition race = make_valid_race();
  race.countdown_duration_s = 3.0;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();
  mgr.start_countdown();

  EXPECT_EQ(mgr.session_state(), p0::race::RaceSessionState::GRID);

  mgr.update(1.0, {}, {}, {}, {}, {});
  EXPECT_EQ(mgr.session_state(), p0::race::RaceSessionState::GRID);

  mgr.update(4.0, {}, {}, {}, {}, {});
  EXPECT_EQ(mgr.session_state(), p0::race::RaceSessionState::FORMATION);
}

TEST(RaceManager, SessionStateAdvancesToGreenFlag) {
  TrackData track = make_valid_track();
  p0::race::RaceDefinition race = make_valid_race();
  race.formation_lap = 1;
  race.countdown_duration_s = 0.0;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();
  mgr.start_countdown();
  mgr.update(1.0, {}, {}, {}, {}, {});
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
  race.countdown_duration_s = 0.0;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  std::unordered_map<int, double> distances;

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();
  mgr.start_countdown();
  mgr.update(1.0, {}, {}, {}, {}, {});

  // Lap 1: progress from 0 to 1000, then wrap to 0
  distances[1] = 0.0;
  mgr.update(1.0, {}, {}, distances, {}, {});
  distances[1] = 200.0;
  mgr.update(2.0, {}, {}, distances, {}, {});
  distances[1] = 400.0;
  mgr.update(3.0, {}, {}, distances, {}, {});
  distances[1] = 600.0;
  mgr.update(4.0, {}, {}, distances, {}, {});
  distances[1] = 800.0;
  mgr.update(5.0, {}, {}, distances, {}, {});
  distances[1] = 0.0;
  mgr.update(6.0, {}, {}, distances, {}, {});

  // Lap 2: progress from 0 to 1000, then wrap to 0
  distances[1] = 200.0;
  mgr.update(7.0, {}, {}, distances, {}, {});
  distances[1] = 400.0;
  mgr.update(8.0, {}, {}, distances, {}, {});
  distances[1] = 600.0;
  mgr.update(9.0, {}, {}, distances, {}, {});
  distances[1] = 800.0;
  mgr.update(10.0, {}, {}, distances, {}, {});
  distances[1] = 0.0;
  mgr.update(11.0, {}, {}, distances, {}, {});

  EXPECT_EQ(mgr.session_state(), p0::race::RaceSessionState::CHECKERED_FLAG);
}

TEST(RaceManager, LapCounterDetectsLapCompletion) {
  TrackData track = make_valid_track();
  track.checkpoints.push_back(Checkpoint{0, {}, 20.0, false, 0});

  p0::race::RaceDefinition race = make_valid_race();
  race.countdown_duration_s = 0.0;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  std::unordered_map<int, double> distances;

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();
  mgr.start_countdown();
  mgr.update(1.0, {}, {}, {}, {}, {});

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
  race.countdown_duration_s = 0.0;
  race.formation_lap = 0;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  a.pit_box_id = 0;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();
  mgr.start_countdown();
  mgr.update(1.0, {}, {}, {}, {}, {});
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

TEST(RaceManager, CountdownEmitsEvents) {
  TrackData track = make_valid_track();
  p0::race::RaceDefinition race = make_valid_race();
  race.countdown_duration_s = 3.0;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();
  mgr.start_countdown();

  mgr.update(1.0, {}, {}, {}, {}, {});
  mgr.update(4.0, {}, {}, {}, {}, {});

  auto events = mgr.drain_events();
  bool has_tick = false;
  bool has_go = false;
  for (const auto& evt : events) {
    if (evt.type == p0::race::RaceEventType::COUNTDOWN_TICK) has_tick = true;
    if (evt.type == p0::race::RaceEventType::COUNTDOWN_GO) has_go = true;
  }
  EXPECT_TRUE(has_tick);
  EXPECT_TRUE(has_go);
}

TEST(RaceManager, FlagStateTransitions) {
  TrackData track = make_valid_track();
  p0::race::RaceDefinition race = make_valid_race();
  race.countdown_duration_s = 0.0;
  race.formation_lap = 0;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();

  EXPECT_EQ(mgr.flag_state(), p0::race::FlagState::GREEN);

  mgr.start_countdown();
  mgr.update(1.0, {}, {}, {}, {}, {});
  mgr.update(200.0, {}, {}, {}, {}, {});

  EXPECT_EQ(mgr.flag_state(), p0::race::FlagState::GREEN);
}

TEST(RaceManager, PostRaceStateAfterAllFinish) {
  TrackData track = make_valid_track();
  track.checkpoints.push_back(Checkpoint{0, {}, 20.0, false, 0});

  p0::race::RaceDefinition race = make_valid_race();
  race.laps = 1;
  race.formation_lap = 0;
  race.countdown_duration_s = 0.0;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  std::unordered_map<int, double> distances;

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();
  mgr.start_countdown();
  mgr.update(1.0, {}, {}, {}, {}, {});

  // Complete 1 lap with smooth progression
  distances[1] = 0.0;
  mgr.update(1.0, {}, {}, distances, {}, {});
  for (int i = 1; i <= 5; ++i) {
    distances[1] = i * 200.0;
    mgr.update(1.0 + i * 10.0, {}, {}, distances, {}, {});
  }
  // Wrap around to complete lap
  distances[1] = 0.0;
  mgr.update(100.0, {}, {}, distances, {}, {});

  // After completing all laps, should be CHECKERED_FLAG or POST_RACE
  auto state = mgr.session_state();
  EXPECT_TRUE(state == p0::race::RaceSessionState::CHECKERED_FLAG ||
              state == p0::race::RaceSessionState::POST_RACE);
}

TEST(RaceManager, PenaltyIssuedForTrackLimits) {
  TrackData track = make_valid_track();
  p0::race::RaceDefinition race = make_valid_race();
  race.countdown_duration_s = 0.0;
  race.formation_lap = 0;
  race.track_limits_strikes = 1;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  std::unordered_map<int, double> distances;
  std::unordered_map<int, Vec2> positions;

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();
  mgr.start_countdown();
  mgr.update(1.0, {}, {}, {}, {}, {});
  mgr.update(200.0, {}, {}, {}, {}, {});
  mgr.update(210.0, {}, {}, {}, {}, {});

  // Verify we're in GREEN_FLAG_RUNNING state
  ASSERT_EQ(mgr.session_state(), p0::race::RaceSessionState::GREEN_FLAG_RUNNING);

  // Car goes off-track (y > width/2 = 7.5)
  positions[1] = p0::Vec2(0.0, 20.0);
  distances[1] = 100.0;
  // Need to stay off-track for off_track_warning_time_s (0.5s)
  mgr.update(3.0, positions, {}, distances, {}, {});
  mgr.update(4.0, positions, {}, distances, {}, {});

  // Check that penalty was issued (may need more time to register)
  auto penalties = mgr.car_active_penalties(1);
  EXPECT_GE(penalties.size(), 1u);
}

TEST(RaceManager, StandingsAvailableDuringRace) {
  TrackData track = make_valid_track();
  p0::race::RaceDefinition race = make_valid_race();
  race.countdown_duration_s = 0.0;
  race.formation_lap = 0;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a;
  a.car_id = 1;
  assignments.push_back(a);

  RaceManager mgr(track, race);
  mgr.initialize(assignments, {});
  mgr.start_race();
  mgr.start_countdown();
  mgr.update(1.0, {}, {}, {}, {}, {});

  // Start formation and green flag
  mgr.update(200.0, {}, {}, {}, {}, {});
  mgr.update(210.0, {}, {}, {}, {}, {});

  auto standings = mgr.current_standings();
  EXPECT_GE(standings.size(), 1u);
}
