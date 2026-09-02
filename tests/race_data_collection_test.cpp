// Project 0 — comprehensive race data collection tests
// Covers: ResultsDatabase population, leaderboard aggregations, JSON
//         persistence, single-result round-trips, edge cases, and time
//         formatting helpers used by the race data collection pipeline.
#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "track/race_results.h"
#include "track/result_storage.h"

using namespace p0;
using p0::track::LapTimeEntry;
using p0::track::LeaderboardEntry;
using p0::track::RaceResult;
using p0::track::ResultsDatabase;
using p0::track::ResultStorage;

namespace {

RaceResult make_finished(const std::string& race_id,
                        const std::string& track_id,
                        const std::string& driver,
                        double total_time,
                        double best_lap,
                        int completed_laps,
                        const std::vector<double>& lap_times = {}) {
  RaceResult r;
  r.race_id = race_id;
  r.track_id = track_id;
  r.track_name = "Track " + track_id;
  r.car_id = 1;
  r.driver_name = driver;
  r.completed_laps = completed_laps;
  r.total_time = total_time;
  r.best_lap_time = best_lap;
  r.finished = true;
  r.dnf = false;
  r.session_date = "2026-08-31T12:00:00Z";
  for (size_t i = 0; i < lap_times.size(); ++i) {
    LapTimeEntry lt;
    lt.lap_time = lap_times[i];
    lt.lap_number = static_cast<int>(i + 1);
    lt.timestamp = static_cast<double>(i) * 30.0;
    lt.valid = true;
    r.lap_times.push_back(lt);
  }
  return r;
}

// Reads the entire file into a string and releases the handle so the file
// can be removed afterwards on Windows without a sharing-violation error.
std::string slurp(const std::string& path) {
  std::ifstream f(path);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

}  // namespace

// ---------------------------------------------------------------------------
// Section 1 — Struct defaults
// ---------------------------------------------------------------------------

TEST(RaceDataCollection, LapTimeEntryDefaults) {
  LapTimeEntry e;
  EXPECT_DOUBLE_EQ(e.lap_time, 0.0);
  EXPECT_TRUE(e.valid);
  EXPECT_EQ(e.lap_number, 0);
  EXPECT_DOUBLE_EQ(e.timestamp, 0.0);
}

TEST(RaceDataCollection, RaceResultDefaults) {
  RaceResult r;
  EXPECT_TRUE(r.race_id.empty());
  EXPECT_TRUE(r.track_id.empty());
  EXPECT_TRUE(r.track_name.empty());
  EXPECT_EQ(r.car_id, 0);
  EXPECT_TRUE(r.driver_name.empty());
  EXPECT_EQ(r.completed_laps, 0);
  EXPECT_DOUBLE_EQ(r.total_time, 0.0);
  EXPECT_DOUBLE_EQ(r.best_lap_time, 0.0);
  EXPECT_TRUE(r.lap_times.empty());
  EXPECT_TRUE(r.session_date.empty());
  EXPECT_FALSE(r.finished);
  EXPECT_FALSE(r.dnf);
  EXPECT_TRUE(r.dnf_reason.empty());
}

TEST(RaceDataCollection, LeaderboardEntryDefaults) {
  LeaderboardEntry e;
  EXPECT_TRUE(e.driver_name.empty());
  EXPECT_EQ(e.car_id, 0);
  EXPECT_DOUBLE_EQ(e.best_lap_time, 0.0);
  EXPECT_DOUBLE_EQ(e.total_time, 0.0);
  EXPECT_EQ(e.races_completed, 0);
  EXPECT_EQ(e.total_valid_laps, 0);
  EXPECT_DOUBLE_EQ(e.avg_lap_time, 0.0);
  EXPECT_EQ(e.position, 0);
}

// ---------------------------------------------------------------------------
// Section 2 — Database CRUD
// ---------------------------------------------------------------------------

TEST(RaceDataCollection, EmptyDatabaseHasNoResults) {
  ResultsDatabase db;
  EXPECT_TRUE(db.all_results().empty());
  EXPECT_TRUE(db.results_for_track("any").empty());
  EXPECT_TRUE(db.results_for_driver("any").empty());
}

TEST(RaceDataCollection, AddMultipleResults) {
  ResultsDatabase db;
  for (int i = 0; i < 5; ++i) {
    db.add_result(make_finished("r" + std::to_string(i), "track_a", "Alice",
                                120.0, 38.0, 3));
  }
  EXPECT_EQ(db.all_results().size(), 5u);
}

TEST(RaceDataCollection, AddPreservesInsertionOrder) {
  ResultsDatabase db;
  db.add_result(make_finished("first",  "t", "A", 100.0, 30.0, 1));
  db.add_result(make_finished("second", "t", "B", 110.0, 35.0, 1));
  db.add_result(make_finished("third",  "t", "C", 120.0, 40.0, 1));
  ASSERT_EQ(db.all_results().size(), 3u);
  EXPECT_EQ(db.all_results()[0].race_id, "first");
  EXPECT_EQ(db.all_results()[1].race_id, "second");
  EXPECT_EQ(db.all_results()[2].race_id, "third");
}

TEST(RaceDataCollection, ClearEmptiesDatabase) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "t", "A", 100.0, 30.0, 1));
  db.add_result(make_finished("r2", "t", "B", 110.0, 35.0, 1));
  EXPECT_EQ(db.all_results().size(), 2u);
  db.clear();
  EXPECT_TRUE(db.all_results().empty());
}

// ---------------------------------------------------------------------------
// Section 3 — Filtering
// ---------------------------------------------------------------------------

TEST(RaceDataCollection, FilterByTrackReturnsOnlyMatching) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "track_a", "Alice", 120.0, 38.0, 3));
  db.add_result(make_finished("r2", "track_b", "Alice", 115.0, 36.0, 3));
  db.add_result(make_finished("r3", "track_a", "Bob",   125.0, 39.0, 3));

  auto a = db.results_for_track("track_a");
  ASSERT_EQ(a.size(), 2u);
  EXPECT_EQ(a[0].driver_name, "Alice");
  EXPECT_EQ(a[1].driver_name, "Bob");

  auto b = db.results_for_track("track_b");
  ASSERT_EQ(b.size(), 1u);
  EXPECT_EQ(b[0].driver_name, "Alice");

  EXPECT_TRUE(db.results_for_track("track_c").empty());
}

TEST(RaceDataCollection, FilterByDriverReturnsAllTracks) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "track_a", "Alice", 120.0, 38.0, 3));
  db.add_result(make_finished("r2", "track_b", "Alice", 115.0, 36.0, 3));
  db.add_result(make_finished("r3", "track_a", "Bob",   125.0, 39.0, 3));

  auto a = db.results_for_driver("Alice");
  EXPECT_EQ(a.size(), 2u);
  EXPECT_EQ(a[0].track_id, "track_a");
  EXPECT_EQ(a[1].track_id, "track_b");

  EXPECT_TRUE(db.results_for_driver("Charlie").empty());
}

TEST(RaceDataCollection, FilterByTrackIgnoresDriver) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "track_a", "Alice", 120.0, 38.0, 3));
  db.add_result(make_finished("r2", "track_b", "Alice", 115.0, 36.0, 3));
  EXPECT_EQ(db.results_for_track("track_a").size(), 1u);
  EXPECT_EQ(db.results_for_track("track_b").size(), 1u);
}

// ---------------------------------------------------------------------------
// Section 4 — Leaderboard aggregations
// ---------------------------------------------------------------------------

TEST(RaceDataCollection, LeaderboardByBestLapOrdersAscending) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "t", "C", 130.0, 42.0, 3));
  db.add_result(make_finished("r2", "t", "A", 120.0, 38.0, 3));
  db.add_result(make_finished("r3", "t", "B", 125.0, 40.0, 3));

  auto lb = db.leaderboard_by_best_lap("t");
  ASSERT_EQ(lb.size(), 3u);
  EXPECT_EQ(lb[0].driver_name, "A");
  EXPECT_EQ(lb[0].position, 1);
  EXPECT_EQ(lb[1].driver_name, "B");
  EXPECT_EQ(lb[1].position, 2);
  EXPECT_EQ(lb[2].driver_name, "C");
  EXPECT_EQ(lb[2].position, 3);
}

TEST(RaceDataCollection, LeaderboardAggregatesMultipleRaces) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "t", "Alice", 110.0, 35.0, 3));
  db.add_result(make_finished("r2", "t", "Alice", 115.0, 38.0, 3));
  db.add_result(make_finished("r3", "t", "Alice", 120.0, 36.0, 3));

  auto lb = db.leaderboard_by_best_lap("t");
  ASSERT_EQ(lb.size(), 1u);
  EXPECT_EQ(lb[0].driver_name, "Alice");
  EXPECT_DOUBLE_EQ(lb[0].best_lap_time, 35.0);
  EXPECT_EQ(lb[0].races_completed, 3);
}

TEST(RaceDataCollection, LeaderboardByTotalTimeSumsRaces) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "t", "Alice", 110.0, 35.0, 3));
  db.add_result(make_finished("r2", "t", "Alice", 115.0, 38.0, 3));

  auto lb = db.leaderboard_by_total_time("t");
  ASSERT_EQ(lb.size(), 1u);
  EXPECT_DOUBLE_EQ(lb[0].total_time, 225.0);
  EXPECT_EQ(lb[0].races_completed, 2);
}

TEST(RaceDataCollection, LeaderboardByAvgLapUsesLapTimes) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "t", "Alice", 120.0, 38.0, 3,
                              {38.0, 42.0, 40.0}));
  db.add_result(make_finished("r2", "t", "Bob",   105.0, 33.0, 3,
                              {33.0, 36.0, 36.0}));

  auto lb = db.leaderboard_by_avg_lap("t");
  ASSERT_EQ(lb.size(), 2u);
  EXPECT_EQ(lb[0].driver_name, "Bob");
  EXPECT_DOUBLE_EQ(lb[0].avg_lap_time, 35.0);
  EXPECT_EQ(lb[1].driver_name, "Alice");
  EXPECT_DOUBLE_EQ(lb[1].avg_lap_time, 40.0);
}

TEST(RaceDataCollection, LeaderboardSkipsUnfinishedRaces) {
  ResultsDatabase db;
  RaceResult dnf;
  dnf.race_id = "r1";
  dnf.track_id = "t";
  dnf.driver_name = "DNF_Driver";
  dnf.best_lap_time = 30.0;       // would otherwise be best
  dnf.total_time = 50.0;
  dnf.finished = false;
  dnf.dnf = true;
  dnf.dnf_reason = "engine";
  db.add_result(dnf);

  db.add_result(make_finished("r2", "t", "Survivor", 120.0, 40.0, 3));

  auto lb = db.leaderboard_by_best_lap("t");
  ASSERT_EQ(lb.size(), 1u);
  EXPECT_EQ(lb[0].driver_name, "Survivor");
}

TEST(RaceDataCollection, LeaderboardIgnoresZeroBestLap) {
  ResultsDatabase db;
  // finished but no best lap time -> excluded
  RaceResult r;
  r.race_id = "r1";
  r.track_id = "t";
  r.driver_name = "NoTime";
  r.finished = true;
  r.completed_laps = 3;
  r.total_time = 0.0;
  r.best_lap_time = 0.0;
  db.add_result(r);

  db.add_result(make_finished("r2", "t", "WithTime", 100.0, 35.0, 3));

  auto lb = db.leaderboard_by_best_lap("t");
  ASSERT_EQ(lb.size(), 1u);
  EXPECT_EQ(lb[0].driver_name, "WithTime");
}

TEST(RaceDataCollection, LeaderboardTrackFilterIsExclusive) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "track_a", "Alice", 120.0, 38.0, 3));
  db.add_result(make_finished("r2", "track_b", "Bob",   115.0, 36.0, 3));

  auto lb_a = db.leaderboard_by_best_lap("track_a");
  EXPECT_EQ(lb_a.size(), 1u);
  EXPECT_EQ(lb_a[0].driver_name, "Alice");

  auto lb_all = db.leaderboard_by_best_lap("");
  EXPECT_EQ(lb_all.size(), 2u);
}

// ---------------------------------------------------------------------------
// Section 5 — Personal best and track record
// ---------------------------------------------------------------------------

TEST(RaceDataCollection, PersonalBestReturnsFastestLap) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "t", "Alice", 110.0, 38.0, 3));
  db.add_result(make_finished("r2", "t", "Alice", 115.0, 36.5, 3));
  db.add_result(make_finished("r3", "t", "Alice", 120.0, 39.0, 3));

  auto pb = db.personal_best("Alice", "t");
  ASSERT_TRUE(pb.has_value());
  EXPECT_DOUBLE_EQ(pb->best_lap_time, 36.5);
  EXPECT_EQ(pb->race_id, "r2");
}

TEST(RaceDataCollection, PersonalBestAcrossTracks) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "track_a", "Alice", 110.0, 38.0, 3));
  db.add_result(make_finished("r2", "track_b", "Alice", 115.0, 35.0, 3));

  auto pb_a = db.personal_best("Alice", "track_a");
  ASSERT_TRUE(pb_a.has_value());
  EXPECT_DOUBLE_EQ(pb_a->best_lap_time, 38.0);

  auto pb_all = db.personal_best("Alice");
  ASSERT_TRUE(pb_all.has_value());
  EXPECT_DOUBLE_EQ(pb_all->best_lap_time, 35.0);
}

TEST(RaceDataCollection, PersonalBestUnknownDriver) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "t", "Alice", 110.0, 38.0, 3));
  EXPECT_FALSE(db.personal_best("Bob", "t").has_value());
}

TEST(RaceDataCollection, TrackRecordReturnsOverallFastest) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "t", "Alice", 110.0, 38.0, 3));
  db.add_result(make_finished("r2", "t", "Bob",   115.0, 36.5, 3));
  db.add_result(make_finished("r3", "t", "Carol", 120.0, 37.0, 3));

  auto tr = db.track_record("t");
  ASSERT_TRUE(tr.has_value());
  EXPECT_EQ(tr->driver_name, "Bob");
  EXPECT_DOUBLE_EQ(tr->best_lap_time, 36.5);
}

TEST(RaceDataCollection, TrackRecordSkipsUnfinished) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "t", "Alice", 110.0, 38.0, 3));
  RaceResult fast_dnf;
  fast_dnf.race_id = "r2";
  fast_dnf.track_id = "t";
  fast_dnf.driver_name = "Crash";
  fast_dnf.best_lap_time = 30.0;
  fast_dnf.finished = false;
  fast_dnf.dnf = true;
  db.add_result(fast_dnf);

  auto tr = db.track_record("t");
  ASSERT_TRUE(tr.has_value());
  EXPECT_EQ(tr->driver_name, "Alice");
}

// ---------------------------------------------------------------------------
// Section 6 — JSON persistence (save / load)
// ---------------------------------------------------------------------------

TEST(RaceDataCollection, SaveAndLoadRoundTripSingleResult) {
  const std::string path = "data/test_results_single.json";
  std::filesystem::create_directories(std::filesystem::path(path).parent_path());

  RaceResult original = make_finished("race_xyz", "track_a", "Alice",
                                      123.456, 38.123, 3,
                                      {38.123, 40.0, 45.333});

  ASSERT_TRUE(ResultStorage::save_race_result(path, original));

  auto loaded = ResultStorage::load_race_result(path);
  ASSERT_TRUE(loaded.has_value());
  EXPECT_EQ(loaded->race_id, "race_xyz");
  EXPECT_EQ(loaded->track_id, "track_a");
  EXPECT_EQ(loaded->track_name, "Track track_a");
  EXPECT_EQ(loaded->driver_name, "Alice");
  EXPECT_DOUBLE_EQ(loaded->total_time, 123.456);
  EXPECT_DOUBLE_EQ(loaded->best_lap_time, 38.123);
  EXPECT_EQ(loaded->completed_laps, 3);
  EXPECT_TRUE(loaded->finished);
  EXPECT_FALSE(loaded->dnf);

  loaded.reset();  // ensure any cached file handle is released
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(RaceDataCollection, SaveAndLoadRoundTripDatabase) {
  ResultsDatabase db;
  db.add_result(make_finished("r1", "track_a", "Alice", 110.0, 36.5, 3,
                              {36.5, 37.0, 36.5}));
  db.add_result(make_finished("r2", "track_b", "Bob",   120.0, 38.0, 3,
                              {38.0, 41.0, 41.0}));

  const std::string path = "data/test_results_db.json";
  std::filesystem::create_directories(std::filesystem::path(path).parent_path());

  ASSERT_TRUE(ResultStorage::save_results(path, db));

  ResultsDatabase loaded;
  ASSERT_TRUE(ResultStorage::load_results(path, loaded));
  // Regression: load_results used to parse the '{' of each LapTimeEntry as
  // an additional RaceResult, so a 2-record save would round-trip as 4+.
  ASSERT_EQ(loaded.all_results().size(), 2u);

  // Sort by race_id for stable comparison.
  auto& r = loaded.all_results();
  std::vector<RaceResult> sorted(r.begin(), r.end());
  std::sort(sorted.begin(), sorted.end(),
            [](const RaceResult& a, const RaceResult& b) {
              return a.race_id < b.race_id;
            });

  EXPECT_EQ(sorted[0].race_id, "r1");
  EXPECT_EQ(sorted[0].driver_name, "Alice");
  EXPECT_DOUBLE_EQ(sorted[0].best_lap_time, 36.5);
  EXPECT_EQ(sorted[0].completed_laps, 3);

  EXPECT_EQ(sorted[1].race_id, "r2");
  EXPECT_EQ(sorted[1].driver_name, "Bob");
  EXPECT_DOUBLE_EQ(sorted[1].best_lap_time, 38.0);

  std::error_code _ec;
  std::filesystem::remove(path, _ec);
}

TEST(RaceDataCollection, LoadResultsClearsExistingDatabase) {
  const std::string path = "data/test_results_clear.json";
  std::filesystem::create_directories(std::filesystem::path(path).parent_path());

  ResultsDatabase db;
  db.add_result(make_finished("r1", "t", "Alice", 100.0, 35.0, 3));
  ASSERT_TRUE(ResultStorage::save_results(path, db));

  ResultsDatabase target;
  target.add_result(make_finished("stale", "t", "Old", 200.0, 50.0, 3));
  EXPECT_EQ(target.all_results().size(), 1u);

  ASSERT_TRUE(ResultStorage::load_results(path, target));
  EXPECT_EQ(target.all_results().size(), 1u);
  EXPECT_EQ(target.all_results()[0].race_id, "r1");

  std::error_code _ec; std::filesystem::remove(path, _ec);
}

TEST(RaceDataCollection, LoadMissingFileReturnsFalse) {
  ResultsDatabase db;
  EXPECT_FALSE(ResultStorage::load_results("Z:/does_not_exist/file.json", db));
  EXPECT_TRUE(db.all_results().empty());
}

TEST(RaceDataCollection, LoadMissingSingleFileReturnsNullopt) {
  EXPECT_FALSE(ResultStorage::load_race_result("Z:/nope/file.json").has_value());
}

TEST(RaceDataCollection, SaveInvalidPathReturnsFalse) {
  RaceResult r;
  EXPECT_FALSE(ResultStorage::save_race_result("Z:/no/where/file.json", r));
  ResultsDatabase db;
  EXPECT_FALSE(ResultStorage::save_results("Z:/no/where/db.json", db));
}

TEST(RaceDataCollection, JsonFileContainsExpectedKeys) {
  const std::string path = "data/test_results_keys.json";
  std::filesystem::create_directories(std::filesystem::path(path).parent_path());

  ResultsDatabase db;
  db.add_result(make_finished("r1", "track_a", "Alice", 110.0, 36.5, 3));
  ASSERT_TRUE(ResultStorage::save_results(path, db));

  const std::string content = slurp(path);

  EXPECT_NE(content.find("\"version\""),       std::string::npos);
  EXPECT_NE(content.find("\"result_count\""),  std::string::npos);
  EXPECT_NE(content.find("\"results\""),       std::string::npos);
  EXPECT_NE(content.find("\"race_id\""),       std::string::npos);
  EXPECT_NE(content.find("\"track_id\""),      std::string::npos);
  EXPECT_NE(content.find("\"driver_name\""),   std::string::npos);
  EXPECT_NE(content.find("\"best_lap_time\""), std::string::npos);

  std::error_code _ec; std::filesystem::remove(path, _ec);
}

// ---------------------------------------------------------------------------
// Section 7 — JSON escaping edge cases
// ---------------------------------------------------------------------------

TEST(RaceDataCollection, JsonEscapesSpecialCharacters) {
  const std::string path = "data/test_results_escape.json";
  std::filesystem::create_directories(std::filesystem::path(path).parent_path());

  RaceResult r;
  r.race_id = "weird\"id\\with\nspecials";
  r.track_id = "track_a";
  r.driver_name = "Quote \"Tester\"";
  r.finished = true;
  r.best_lap_time = 40.0;
  r.total_time = 120.0;

  ASSERT_TRUE(ResultStorage::save_race_result(path, r));

  // Validate the escaped form on disk before we round-trip.
  const std::string content = slurp(path);

  // The embedded literal quotes must be escaped (\"), the literal
  // backslash must be doubled, the newline must appear as \n.
  EXPECT_NE(content.find("weird\\\"id\\\\with\\nspecials"), std::string::npos);
  EXPECT_NE(content.find("Quote \\\"Tester\\\""), std::string::npos);

  auto loaded = ResultStorage::load_race_result(path);
  ASSERT_TRUE(loaded.has_value());
  EXPECT_EQ(loaded->race_id, "weird\"id\\with\nspecials");
  EXPECT_EQ(loaded->driver_name, "Quote \"Tester\"");

  std::error_code _ec; std::filesystem::remove(path, _ec);
}

// ---------------------------------------------------------------------------
// Section 8 — Helper functions
// ---------------------------------------------------------------------------

TEST(RaceDataCollection, FormatTimeHandlesZeroAndNegative) {
  EXPECT_EQ(ResultStorage::format_time(0.0),   "--:--.---");
  EXPECT_EQ(ResultStorage::format_time(-1.0),  "--:--.---");
}

TEST(RaceDataCollection, FormatTimeFormatsMinutesAndSeconds) {
  EXPECT_EQ(ResultStorage::format_time(83.5), "01:23.500");
  EXPECT_EQ(ResultStorage::format_time(125.250), "02:05.250");
  EXPECT_EQ(ResultStorage::format_time(9.999), "00:09.999");
}

TEST(RaceDataCollection, CurrentTimestampHasIsoShape) {
  const std::string ts = ResultStorage::current_timestamp();
  // YYYY-MM-DDTHH:MM:SSZ
  EXPECT_EQ(ts.size(), 20u);
  EXPECT_EQ(ts[4],  '-');
  EXPECT_EQ(ts[7],  '-');
  EXPECT_EQ(ts[10], 'T');
  EXPECT_EQ(ts[13], ':');
  EXPECT_EQ(ts[16], ':');
  EXPECT_EQ(ts[19], 'Z');
}

TEST(RaceDataCollection, GenerateRaceIdIncludesTrackCarAndTimestamp) {
  std::string id = ResultStorage::generate_race_id("monza", 7);
  EXPECT_NE(id.find("monza"),    std::string::npos);
  EXPECT_NE(id.find("car7"),     std::string::npos);
  EXPECT_GE(id.size(), std::string("monza_car7_").size());
}

TEST(RaceDataCollection, DefaultResultsPathIsStable) {
  EXPECT_EQ(ResultStorage::default_results_path(), "data/results.json");
}

// ---------------------------------------------------------------------------
// Section 9 — Integration scenarios
// ---------------------------------------------------------------------------

TEST(RaceDataCollection, ThreeDriverChampionshipOnOneTrack) {
  ResultsDatabase db;

  // Each driver runs two races.
  db.add_result(make_finished("r1", "monza", "Alice", 300.0, 95.0, 5));
  db.add_result(make_finished("r2", "monza", "Alice", 305.0, 96.0, 5));
  db.add_result(make_finished("r3", "monza", "Bob",   295.0, 92.0, 5));
  db.add_result(make_finished("r4", "monza", "Bob",   298.0, 93.0, 5));
  db.add_result(make_finished("r5", "monza", "Carol", 310.0, 98.0, 5));
  db.add_result(make_finished("r6", "monza", "Carol", 312.0, 97.0, 5));

  auto lb_best = db.leaderboard_by_best_lap("monza");
  ASSERT_EQ(lb_best.size(), 3u);
  EXPECT_EQ(lb_best[0].driver_name, "Bob");
  EXPECT_EQ(lb_best[0].position, 1);
  EXPECT_DOUBLE_EQ(lb_best[0].best_lap_time, 92.0);
  EXPECT_EQ(lb_best[0].races_completed, 2);

  auto lb_total = db.leaderboard_by_total_time("monza");
  ASSERT_EQ(lb_total.size(), 3u);
  // Bob has the lowest total (593), Alice 605, Carol 622.
  EXPECT_EQ(lb_total[0].driver_name, "Bob");
  EXPECT_NEAR(lb_total[0].total_time, 593.0, 1e-6);

  auto tr = db.track_record("monza");
  ASSERT_TRUE(tr.has_value());
  EXPECT_EQ(tr->driver_name, "Bob");
  EXPECT_DOUBLE_EQ(tr->best_lap_time, 92.0);
}

TEST(RaceDataCollection, LapTimesPreservedThroughRoundTrip) {
  const std::string path = "data/test_results_laptimes.json";
  std::filesystem::create_directories(std::filesystem::path(path).parent_path());

  ResultsDatabase db;
  // 3 default lap times from make_finished, plus 1 custom entry.
  RaceResult r = make_finished("r1", "t", "Alice", 120.0, 38.0, 3,
                                {38.0, 40.0, 38.0});
  LapTimeEntry lt;
  lt.lap_number = 4;
  lt.lap_time = 39.5;
  lt.valid = false;
  lt.timestamp = 100.5;
  r.lap_times.push_back(lt);
  db.add_result(r);

  ASSERT_TRUE(ResultStorage::save_results(path, db));

  ResultsDatabase loaded;
  ASSERT_TRUE(ResultStorage::load_results(path, loaded));
  ASSERT_EQ(loaded.all_results().size(), 1u);

  const auto& loaded_r = loaded.all_results()[0];
  EXPECT_EQ(loaded_r.driver_name, "Alice");
  EXPECT_DOUBLE_EQ(loaded_r.best_lap_time, 38.0);
  ASSERT_EQ(loaded_r.lap_times.size(), 4u);

  // The custom 4th entry (lap 4, invalid) should be preserved.
  const auto& custom = loaded_r.lap_times.back();
  EXPECT_EQ(custom.lap_number, 4);
  EXPECT_DOUBLE_EQ(custom.lap_time, 39.5);
  EXPECT_FALSE(custom.valid);
  EXPECT_DOUBLE_EQ(custom.timestamp, 100.5);

  std::error_code _ec;
  std::filesystem::remove(path, _ec);
}

TEST(RaceDataCollection, RepeatedSaveIsIdempotent) {
  const std::string path = "data/test_results_idempotent.json";
  std::filesystem::create_directories(std::filesystem::path(path).parent_path());

  ResultsDatabase db;
  db.add_result(make_finished("r1", "t", "Alice", 100.0, 35.0, 3));
  ASSERT_TRUE(ResultStorage::save_results(path, db));
  ASSERT_TRUE(ResultStorage::save_results(path, db));

  ResultsDatabase loaded;
  ASSERT_TRUE(ResultStorage::load_results(path, loaded));
  EXPECT_EQ(loaded.all_results().size(), 1u);

  std::error_code _ec; std::filesystem::remove(path, _ec);
}
