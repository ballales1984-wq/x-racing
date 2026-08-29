#include <gtest/gtest.h>
#include "track/result_storage.h"
#include "track/race_results.h"
#include <filesystem>

using namespace p0::track;

TEST(ResultStorage, FormatTimePositive) {
  std::string s = ResultStorage::format_time(125.5);
  EXPECT_EQ(s, "02:05.500");
}

TEST(ResultStorage, FormatTimeZeroReturnsDash) {
  std::string s = ResultStorage::format_time(0.0);
  EXPECT_EQ(s, "--:--.---");
}

TEST(ResultStorage, FormatTimeNegativeReturnsDash) {
  std::string s = ResultStorage::format_time(-5.0);
  EXPECT_EQ(s, "--:--.---");
}

TEST(ResultStorage, FormatTimeUnderMinute) {
  std::string s = ResultStorage::format_time(45.123);
  EXPECT_EQ(s, "00:45.123");
}

TEST(ResultStorage, GenerateRaceId) {
  std::string id = ResultStorage::generate_race_id("monza", 7);
  EXPECT_NE(id.find("monza"), std::string::npos);
  EXPECT_NE(id.find("car7"), std::string::npos);
}

TEST(ResultStorage, CurrentTimestampFormat) {
  std::string ts = ResultStorage::current_timestamp();
  EXPECT_EQ(ts.size(), 20u);
  EXPECT_EQ(ts[4], '-');
  EXPECT_EQ(ts[10], 'T');
  EXPECT_EQ(ts[13], ':');
  EXPECT_EQ(ts[16], ':');
  EXPECT_EQ(ts[19], 'Z');
}

TEST(ResultStorage, SaveAndLoadResultsRoundTrip) {
  std::string path = std::string(std::getenv("TEMP")) + "/p0_results_test.json";
  if (std::filesystem::exists(path)) std::filesystem::remove(path);

  ResultsDatabase db;
  RaceResult r;
  r.race_id = "r1";
  r.track_id = "monza";
  r.track_name = "Monza";
  r.car_id = 1;
  r.driver_name = "Driver";
  r.completed_laps = 5;
  r.total_time = 300.0;
  r.best_lap_time = 60.0;
  r.finished = true;
  db.add_result(r);

  bool saved = ResultStorage::save_results(path, db);
  EXPECT_TRUE(saved);

  ResultsDatabase loaded;
  bool loaded_ok = ResultStorage::load_results(path, loaded);
  EXPECT_TRUE(loaded_ok);
  ASSERT_EQ(loaded.all_results().size(), 1u);
  EXPECT_EQ(loaded.all_results()[0].race_id, "r1");
  EXPECT_EQ(loaded.all_results()[0].track_id, "monza");
  EXPECT_EQ(loaded.all_results()[0].completed_laps, 5);

  std::filesystem::remove(path);
}

TEST(ResultStorage, SaveAndLoadSingleRaceResultRoundTrip) {
  std::string path = std::string(std::getenv("TEMP")) + "/p0_race_result_test.json";
  if (std::filesystem::exists(path)) std::filesystem::remove(path);

  RaceResult r;
  r.race_id = "r2";
  r.track_id = "silverstone";
  r.car_id = 3;
  r.driver_name = "Alice";
  r.completed_laps = 10;
  r.total_time = 1200.0;
  r.best_lap_time = 115.5;
  r.finished = true;

  bool saved = ResultStorage::save_race_result(path, r);
  EXPECT_TRUE(saved);

  auto loaded = ResultStorage::load_race_result(path);
  ASSERT_TRUE(loaded.has_value());
  EXPECT_EQ(loaded->race_id, "r2");
  EXPECT_EQ(loaded->track_id, "silverstone");
  EXPECT_EQ(loaded->car_id, 3);
  EXPECT_EQ(loaded->driver_name, "Alice");
  EXPECT_DOUBLE_EQ(loaded->total_time, 1200.0);

  std::filesystem::remove(path);
}

TEST(ResultStorage, LoadMissingFileReturnsFalse) {
  ResultsDatabase db;
  bool ok = ResultStorage::load_results("missing_file_xyz.json", db);
  EXPECT_FALSE(ok);
}

TEST(ResultStorage, LoadRaceResultMissingReturnsNullopt) {
  auto r = ResultStorage::load_race_result("missing_file_xyz.json");
  EXPECT_FALSE(r.has_value());
}
