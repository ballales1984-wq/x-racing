// Project 0 — unit tests for race results and leaderboard logic
#include <gtest/gtest.h>
#include "track/race_results.h"

using namespace p0;

TEST(ResultsDatabase, AddResult) {
  track::ResultsDatabase db;
  track::RaceResult r;
  r.race_id = "test_race_1";
  r.track_id = "track_a";
  r.track_name = "Test Track A";
  r.car_id = 1;
  r.driver_name = "Driver A";
  r.completed_laps = 3;
  r.total_time = 120.5;
  r.best_lap_time = 38.2;
  r.finished = true;

  db.add_result(r);
  ASSERT_EQ(db.all_results().size(), 1u);
  EXPECT_EQ(db.all_results()[0].race_id, "test_race_1");
}

TEST(ResultsDatabase, ClearRemovesAll) {
  track::ResultsDatabase db;
  track::RaceResult r;
  r.race_id = "r1";
  r.track_id = "t1";
  r.finished = true;
  r.best_lap_time = 40.0;

  db.add_result(r);
  db.add_result(r);
  db.clear();
  EXPECT_TRUE(db.all_results().empty());
}

TEST(ResultsDatabase, FilterByTrack) {
  track::ResultsDatabase db;
  track::RaceResult r1;
  r1.race_id = "r1";
  r1.track_id = "track_a";
  r1.finished = true;
  r1.best_lap_time = 40.0;

  track::RaceResult r2;
  r2.race_id = "r2";
  r2.track_id = "track_b";
  r2.finished = true;
  r2.best_lap_time = 35.0;

  db.add_result(r1);
  db.add_result(r2);

  auto track_a = db.results_for_track("track_a");
  ASSERT_EQ(track_a.size(), 1u);
  EXPECT_EQ(track_a[0].track_id, "track_a");

  auto track_b = db.results_for_track("track_b");
  ASSERT_EQ(track_b.size(), 1u);
  EXPECT_EQ(track_b[0].track_id, "track_b");
}

TEST(ResultsDatabase, FilterByDriver) {
  track::ResultsDatabase db;
  track::RaceResult r1;
  r1.race_id = "r1";
  r1.driver_name = "Alice";
  r1.finished = true;
  r1.best_lap_time = 40.0;

  track::RaceResult r2;
  r2.race_id = "r2";
  r2.driver_name = "Bob";
  r2.finished = true;
  r2.best_lap_time = 35.0;

  db.add_result(r1);
  db.add_result(r2);

  auto alice = db.results_for_driver("Alice");
  ASSERT_EQ(alice.size(), 1u);
  EXPECT_EQ(alice[0].driver_name, "Alice");

  auto bob = db.results_for_driver("Bob");
  ASSERT_EQ(bob.size(), 1u);
  EXPECT_EQ(bob[0].driver_name, "Bob");
}

TEST(ResultsDatabase, LeaderboardByBestLap) {
  track::ResultsDatabase db;

  track::RaceResult r1;
  r1.race_id = "r1";
  r1.track_id = "track_a";
  r1.driver_name = "Alice";
  r1.completed_laps = 3;
  r1.total_time = 120.0;
  r1.best_lap_time = 38.0;
  r1.finished = true;

  track::RaceResult r2;
  r2.race_id = "r2";
  r2.track_id = "track_a";
  r2.driver_name = "Bob";
  r2.completed_laps = 3;
  r2.total_time = 115.0;
  r2.best_lap_time = 36.5;
  r2.finished = true;

  track::RaceResult r3;
  r3.race_id = "r3";
  r3.track_id = "track_b";
  r3.driver_name = "Charlie";
  r3.completed_laps = 3;
  r3.total_time = 110.0;
  r3.best_lap_time = 35.0;
  r3.finished = true;

  db.add_result(r1);
  db.add_result(r2);
  db.add_result(r3);

  auto lb_a = db.leaderboard_by_best_lap("track_a");
  ASSERT_EQ(lb_a.size(), 2u);
  EXPECT_EQ(lb_a[0].driver_name, "Bob");
  EXPECT_EQ(lb_a[0].best_lap_time, 36.5);
  EXPECT_EQ(lb_a[0].position, 1);
  EXPECT_EQ(lb_a[1].driver_name, "Alice");
  EXPECT_EQ(lb_a[1].best_lap_time, 38.0);
  EXPECT_EQ(lb_a[1].position, 2);

  auto lb_b = db.leaderboard_by_best_lap("track_b");
  ASSERT_EQ(lb_b.size(), 1u);
  EXPECT_EQ(lb_b[0].driver_name, "Charlie");
}

TEST(ResultsDatabase, LeaderboardByTotalTime) {
  track::ResultsDatabase db;

  track::RaceResult r1;
  r1.race_id = "r1";
  r1.track_id = "track_a";
  r1.driver_name = "Alice";
  r1.completed_laps = 3;
  r1.total_time = 120.0;
  r1.best_lap_time = 38.0;
  r1.finished = true;

  track::RaceResult r2;
  r2.race_id = "r2";
  r2.track_id = "track_a";
  r2.driver_name = "Alice";
  r2.completed_laps = 3;
  r2.total_time = 110.0;
  r2.best_lap_time = 36.0;
  r2.finished = true;

  db.add_result(r1);
  db.add_result(r2);

  auto lb = db.leaderboard_by_total_time("track_a");
  ASSERT_EQ(lb.size(), 1u);
  EXPECT_EQ(lb[0].driver_name, "Alice");
  EXPECT_DOUBLE_EQ(lb[0].total_time, 230.0);
  EXPECT_EQ(lb[0].races_completed, 2);
}

TEST(ResultsDatabase, LeaderboardByAvgLap) {
  track::ResultsDatabase db;

  track::RaceResult r1;
  r1.race_id = "r1";
  r1.track_id = "track_a";
  r1.driver_name = "Alice";
  r1.completed_laps = 3;
  r1.total_time = 120.0;
  r1.best_lap_time = 38.0;
  r1.finished = true;
  r1.lap_times = {{38.0, true, 1, 0}, {42.0, true, 2, 0}, {40.0, true, 3, 0}};

  track::RaceResult r2;
  r2.race_id = "r2";
  r2.track_id = "track_a";
  r2.driver_name = "Bob";
  r2.completed_laps = 3;
  r2.total_time = 105.0;
  r2.best_lap_time = 33.0;
  r2.finished = true;
  r2.lap_times = {{33.0, true, 1, 0}, {36.0, true, 2, 0}, {36.0, true, 3, 0}};

  db.add_result(r1);
  db.add_result(r2);

  auto lb = db.leaderboard_by_avg_lap("track_a");
  ASSERT_EQ(lb.size(), 2u);
  EXPECT_DOUBLE_EQ(lb[0].avg_lap_time, 35.0);
  EXPECT_EQ(lb[0].driver_name, "Bob");
  EXPECT_DOUBLE_EQ(lb[1].avg_lap_time, 40.0);
  EXPECT_EQ(lb[1].driver_name, "Alice");
}

TEST(ResultsDatabase, PersonalBest) {
  track::ResultsDatabase db;

  track::RaceResult r1;
  r1.race_id = "r1";
  r1.track_id = "track_a";
  r1.driver_name = "Alice";
  r1.best_lap_time = 38.0;
  r1.finished = true;

  track::RaceResult r2;
  r2.race_id = "r2";
  r2.track_id = "track_a";
  r2.driver_name = "Alice";
  r2.best_lap_time = 36.5;
  r2.finished = true;

  track::RaceResult r3;
  r3.race_id = "r3";
  r3.track_id = "track_b";
  r3.driver_name = "Alice";
  r3.best_lap_time = 35.0;
  r3.finished = true;

  db.add_result(r1);
  db.add_result(r2);
  db.add_result(r3);

  auto pb_a = db.personal_best("Alice", "track_a");
  ASSERT_TRUE(pb_a.has_value());
  EXPECT_DOUBLE_EQ(pb_a->best_lap_time, 36.5);

  auto pb_b = db.personal_best("Alice", "track_b");
  ASSERT_TRUE(pb_b.has_value());
  EXPECT_DOUBLE_EQ(pb_b->best_lap_time, 35.0);

  auto pb_all = db.personal_best("Alice");
  ASSERT_TRUE(pb_all.has_value());
  EXPECT_DOUBLE_EQ(pb_all->best_lap_time, 35.0);
}

TEST(ResultsDatabase, TrackRecord) {
  track::ResultsDatabase db;

  track::RaceResult r1;
  r1.race_id = "r1";
  r1.track_id = "track_a";
  r1.driver_name = "Alice";
  r1.best_lap_time = 38.0;
  r1.finished = true;

  track::RaceResult r2;
  r2.race_id = "r2";
  r2.track_id = "track_a";
  r2.driver_name = "Bob";
  r2.best_lap_time = 36.5;
  r2.finished = true;

  track::RaceResult r3;
  r3.race_id = "r3";
  r3.track_id = "track_b";
  r3.driver_name = "Charlie";
  r3.best_lap_time = 35.0;
  r3.finished = true;

  db.add_result(r1);
  db.add_result(r2);
  db.add_result(r3);

  auto tr_a = db.track_record("track_a");
  ASSERT_TRUE(tr_a.has_value());
  EXPECT_DOUBLE_EQ(tr_a->best_lap_time, 36.5);
  EXPECT_EQ(tr_a->driver_name, "Bob");

  auto tr_b = db.track_record("track_b");
  ASSERT_TRUE(tr_b.has_value());
  EXPECT_DOUBLE_EQ(tr_b->best_lap_time, 35.0);
}

TEST(ResultsDatabase, SkipsUnfinishedResults) {
  track::ResultsDatabase db;

  track::RaceResult r1;
  r1.race_id = "r1";
  r1.track_id = "track_a";
  r1.driver_name = "Alice";
  r1.best_lap_time = 38.0;
  r1.finished = false;

  track::RaceResult r2;
  r2.race_id = "r2";
  r2.track_id = "track_a";
  r2.driver_name = "Bob";
  r2.best_lap_time = 36.0;
  r2.finished = true;

  db.add_result(r1);
  db.add_result(r2);

  auto lb = db.leaderboard_by_best_lap("track_a");
  ASSERT_EQ(lb.size(), 1u);
  EXPECT_EQ(lb[0].driver_name, "Bob");
}

TEST(ResultsDatabase, SkipsInvalidBestLap) {
  track::ResultsDatabase db;

  track::RaceResult r1;
  r1.race_id = "r1";
  r1.track_id = "track_a";
  r1.driver_name = "Alice";
  r1.best_lap_time = 0.0;
  r1.finished = true;

  track::RaceResult r2;
  r2.race_id = "r2";
  r2.track_id = "track_a";
  r2.driver_name = "Bob";
  r2.best_lap_time = 36.0;
  r2.finished = true;

  db.add_result(r1);
  db.add_result(r2);

  auto lb = db.leaderboard_by_best_lap("track_a");
  ASSERT_EQ(lb.size(), 1u);
  EXPECT_EQ(lb[0].driver_name, "Bob");
}

TEST(ResultsDatabase, MultiRaceAggregation) {
  track::ResultsDatabase db;

  for (int i = 0; i < 3; ++i) {
    track::RaceResult r;
    r.race_id = "r" + std::to_string(i);
    r.track_id = "track_a";
    r.driver_name = "Alice";
    r.completed_laps = 3;
    r.total_time = 110.0 + i * 2;
    r.best_lap_time = 35.0 + i;
    r.finished = true;
    r.lap_times = {{35.0 + i, true, 1, 0}, {36.0 + i, true, 2, 0}, {37.0 + i, true, 3, 0}};
    db.add_result(r);
  }

  auto lb = db.leaderboard_by_best_lap("track_a");
  ASSERT_EQ(lb.size(), 1u);
  EXPECT_EQ(lb[0].driver_name, "Alice");
  EXPECT_DOUBLE_EQ(lb[0].best_lap_time, 35.0);
  EXPECT_EQ(lb[0].races_completed, 3);
  EXPECT_EQ(lb[0].total_valid_laps, 9);
}

TEST(ResultsDatabase, EmptyLeaderboard) {
  track::ResultsDatabase db;
  auto lb = db.leaderboard_by_best_lap("track_a");
  EXPECT_TRUE(lb.empty());

  auto pb = db.personal_best("Alice");
  EXPECT_FALSE(pb.has_value());

  auto tr = db.track_record("track_a");
  EXPECT_FALSE(tr.has_value());
}

TEST(ResultsDatabase, DNFExcludedFromLeaderboard) {
  track::ResultsDatabase db;

  track::RaceResult r1;
  r1.race_id = "r1";
  r1.track_id = "track_a";
  r1.driver_name = "Alice";
  r1.best_lap_time = 38.0;
  r1.finished = true;

  track::RaceResult r2;
  r2.race_id = "r2";
  r2.track_id = "track_a";
  r2.driver_name = "Bob";
  r2.best_lap_time = 36.0;
  r2.finished = false;
  r2.dnf = true;
  r2.dnf_reason = "Engine failure";

  db.add_result(r1);
  db.add_result(r2);

  auto lb = db.leaderboard_by_best_lap("track_a");
  ASSERT_EQ(lb.size(), 1u);
  EXPECT_EQ(lb[0].driver_name, "Alice");
}

TEST(LapTimeEntry, DefaultValues) {
  track::LapTimeEntry lt;
  EXPECT_DOUBLE_EQ(lt.lap_time, 0.0);
  EXPECT_TRUE(lt.valid);
  EXPECT_EQ(lt.lap_number, 0);
  EXPECT_DOUBLE_EQ(lt.timestamp, 0.0);
}

TEST(RaceResult, DefaultValues) {
  track::RaceResult r;
  EXPECT_TRUE(r.race_id.empty());
  EXPECT_TRUE(r.track_id.empty());
  EXPECT_EQ(r.car_id, 0);
  EXPECT_EQ(r.completed_laps, 0);
  EXPECT_DOUBLE_EQ(r.total_time, 0.0);
  EXPECT_FALSE(r.finished);
  EXPECT_FALSE(r.dnf);
}

TEST(LeaderboardEntry, DefaultValues) {
  track::LeaderboardEntry entry;
  EXPECT_TRUE(entry.driver_name.empty());
  EXPECT_EQ(entry.position, 0);
  EXPECT_EQ(entry.races_completed, 0);
}

TEST(LeaderboardByBestLap, PositionsAssigned) {
  track::ResultsDatabase db;

  track::RaceResult r1;
  r1.race_id = "r1";
  r1.track_id = "track_a";
  r1.driver_name = "A";
  r1.best_lap_time = 40.0;
  r1.finished = true;

  track::RaceResult r2;
  r2.race_id = "r2";
  r2.track_id = "track_a";
  r2.driver_name = "B";
  r2.best_lap_time = 38.0;
  r2.finished = true;

  track::RaceResult r3;
  r3.race_id = "r3";
  r3.track_id = "track_a";
  r3.driver_name = "C";
  r3.best_lap_time = 42.0;
  r3.finished = true;

  db.add_result(r1);
  db.add_result(r2);
  db.add_result(r3);

  auto lb = db.leaderboard_by_best_lap("track_a");
  ASSERT_EQ(lb.size(), 3u);
  EXPECT_EQ(lb[0].position, 1);
  EXPECT_EQ(lb[1].position, 2);
  EXPECT_EQ(lb[2].position, 3);
}
