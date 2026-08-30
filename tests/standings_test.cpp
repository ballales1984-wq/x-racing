#include <gtest/gtest.h>
#include "track/standings.h"

using namespace p0::track;
using p0::race::RaceSessionState;

TEST(StandingsTracker, EmptyByDefault) {
  StandingsTracker tracker;
  auto standings = tracker.current_standings();
  EXPECT_TRUE(standings.empty());
}

TEST(StandingsTracker, GridAssignmentOrder) {
  StandingsTracker tracker;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a1; a1.car_id = 1; a1.grid_slot = 1;
  p0::race::CarAssignment a2; a2.car_id = 2; a2.grid_slot = 2;
  p0::race::CarAssignment a3; a3.car_id = 3; a3.grid_slot = 3;
  assignments.push_back(a1);
  assignments.push_back(a2);
  assignments.push_back(a3);

  std::vector<GridSlot> grid_slots;
  grid_slots.push_back(GridSlot{0, {}, 4.0, 10.0});
  grid_slots.push_back(GridSlot{1, {}, 4.0, 10.0});
  grid_slots.push_back(GridSlot{2, {}, 4.0, 10.0});

  tracker.set_grid(assignments, grid_slots);

  std::unordered_map<int, double> finished;
  std::unordered_map<int, int> laps;
  laps[1] = 0; laps[2] = 0; laps[3] = 0;
  tracker.update(finished, laps);

  auto standings = tracker.current_standings();
  EXPECT_EQ(standings.size(), 3u);
  EXPECT_EQ(standings[0].position, 1);
  EXPECT_EQ(standings[1].position, 2);
  EXPECT_EQ(standings[2].position, 3);
}

TEST(StandingsTracker, LapAdvancementChangesPosition) {
  StandingsTracker tracker;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a1; a1.car_id = 1; a1.grid_slot = 1;
  p0::race::CarAssignment a2; a2.car_id = 2; a2.grid_slot = 2;
  assignments.push_back(a1);
  assignments.push_back(a2);

  std::vector<GridSlot> grid_slots;
  grid_slots.push_back(GridSlot{0, {}, 4.0, 10.0});
  grid_slots.push_back(GridSlot{1, {}, 4.0, 10.0});

  tracker.set_grid(assignments, grid_slots);

  std::unordered_map<int, double> finished;
  std::unordered_map<int, int> laps;
  laps[1] = 0; laps[2] = 1;
  tracker.update(finished, laps);

  auto standings = tracker.current_standings();
  EXPECT_EQ(standings[0].car_id, 2);
  EXPECT_EQ(standings[0].position, 1);
  EXPECT_EQ(standings[1].car_id, 1);
  EXPECT_EQ(standings[1].position, 2);
}

TEST(StandingsTracker, BestLapTracking) {
  StandingsTracker tracker;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a1; a1.car_id = 1; a1.grid_slot = 1;
  assignments.push_back(a1);

  std::vector<GridSlot> grid_slots;
  grid_slots.push_back(GridSlot{0, {}, 4.0, 10.0});

  tracker.set_grid(assignments, grid_slots);

  tracker.record_lap(1, 90.5, true);
  tracker.record_lap(1, 88.2, true);
  tracker.record_lap(1, 89.0, true);

  auto entry = tracker.get_car(1);
  EXPECT_DOUBLE_EQ(entry.best_lap_time, 88.2);
}

TEST(StandingsTracker, GapCalculation) {
  StandingsTracker tracker;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a1; a1.car_id = 1; a1.grid_slot = 1;
  p0::race::CarAssignment a2; a2.car_id = 2; a2.grid_slot = 2;
  assignments.push_back(a1);
  assignments.push_back(a2);

  std::vector<GridSlot> grid_slots;
  grid_slots.push_back(GridSlot{0, {}, 4.0, 10.0});
  grid_slots.push_back(GridSlot{1, {}, 4.0, 10.0});

  tracker.set_grid(assignments, grid_slots);

  tracker.record_lap(1, 90.0, true);
  tracker.record_lap(2, 92.0, true);

  std::unordered_map<int, double> finished;
  std::unordered_map<int, int> laps;
  laps[1] = 1; laps[2] = 1;
  tracker.update(finished, laps);

  auto standings = tracker.current_standings();
  EXPECT_EQ(standings[0].car_id, 1);
  EXPECT_DOUBLE_EQ(standings[0].gap_to_leader_s, 0.0);
  EXPECT_EQ(standings[1].car_id, 2);
  EXPECT_DOUBLE_EQ(standings[1].gap_to_leader_s, 2.0);
}

TEST(StandingsTracker, FinishMarksCompleted) {
  StandingsTracker tracker;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a1; a1.car_id = 1; a1.grid_slot = 1;
  assignments.push_back(a1);

  std::vector<GridSlot> grid_slots;
  grid_slots.push_back(GridSlot{0, {}, 4.0, 10.0});

  tracker.set_grid(assignments, grid_slots);

  std::unordered_map<int, double> finished;
  finished[1] = 270.0;
  std::unordered_map<int, int> laps;
  laps[1] = 3;
  tracker.update(finished, laps);

  auto entry = tracker.get_car(1);
  EXPECT_TRUE(entry.finished);
}

TEST(StandingsTracker, DNFExcludedFromStandings) {
  StandingsTracker tracker;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a1; a1.car_id = 1; a1.grid_slot = 1;
  p0::race::CarAssignment a2; a2.car_id = 2; a2.grid_slot = 2;
  assignments.push_back(a1);
  assignments.push_back(a2);

  std::vector<GridSlot> grid_slots;
  grid_slots.push_back(GridSlot{0, {}, 4.0, 10.0});
  grid_slots.push_back(GridSlot{1, {}, 4.0, 10.0});

  tracker.set_grid(assignments, grid_slots);

  tracker.mark_dnf(2, "Engine failure");

  std::unordered_map<int, double> finished;
  std::unordered_map<int, int> laps;
  laps[1] = 2; laps[2] = 1;
  tracker.update(finished, laps);

  auto standings = tracker.current_standings();
  EXPECT_EQ(standings[0].car_id, 1);
  EXPECT_EQ(standings[1].car_id, 2);
  EXPECT_TRUE(standings[1].dnf);
  EXPECT_EQ(standings[1].dnf_reason, "Engine failure");
}

TEST(StandingsTracker, ResetClearsAll) {
  StandingsTracker tracker;

  std::vector<p0::race::CarAssignment> assignments;
  p0::race::CarAssignment a1; a1.car_id = 1; a1.grid_slot = 1;
  assignments.push_back(a1);

  std::vector<GridSlot> grid_slots;
  grid_slots.push_back(GridSlot{0, {}, 4.0, 10.0});

  tracker.set_grid(assignments, grid_slots);
  tracker.reset();

  auto standings = tracker.current_standings();
  EXPECT_TRUE(standings.empty());
}
