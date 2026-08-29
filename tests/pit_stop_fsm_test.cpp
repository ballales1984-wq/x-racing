#include <gtest/gtest.h>
#include "track/pit_stop_fsm.h"
#include "track/pit_lane.h"

using namespace p0::track;

TEST(PitStopFSM, InitialStateIsNone) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem system(def);
  PitStopFSM fsm(1, system);

  EXPECT_EQ(fsm.state(), p0::race::PitStopState::NONE);
  EXPECT_FALSE(fsm.is_active());
  EXPECT_FALSE(fsm.is_complete());
}

TEST(PitStopFSM, RequestStopTransitionsToRequested) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem system(def);
  PitStopFSM fsm(1, system);

  fsm.request_stop(p0::race::TireCompound::SOFT, true, true, false);
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::REQUESTED);
  EXPECT_TRUE(fsm.is_active());
  EXPECT_FALSE(fsm.is_complete());
}

TEST(PitStopFSM, RequestStopOnlyFromNone) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem system(def);
  PitStopFSM fsm(1, system);

  fsm.request_stop(p0::race::TireCompound::SOFT, true, true, false);
  fsm.request_stop(p0::race::TireCompound::HARD, false, false, true);
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::REQUESTED);
}

TEST(PitStopFSM, AbandonTransitionsToAbandoned) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem system(def);
  PitStopFSM fsm(1, system);

  fsm.request_stop(p0::race::TireCompound::SOFT, true, true, false);
  fsm.abandon();
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::ABANDONED);
  EXPECT_TRUE(fsm.is_complete());
  EXPECT_FALSE(fsm.is_active());
}

TEST(PitStopFSM, AbandonOnlyFromNone) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem ps(def);
  PitStopFSM fsm(1, ps);

  fsm.abandon();
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::ABANDONED);
}

TEST(PitStopFSM, CompleteAfterTrackReentry) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  PitBox box{0, 0, {}, {}, {}, {}, 4.0, 6.0};
  box.position.position = p0::Vec2(50.0, 0.0);
  def.boxes.push_back(box);
  def.speed_zone.speed_limit_m_s = 16.67;
  def.exit.transform.position = p0::Vec2(100.0, 0.0);
  PitLaneSystem ps(def);
  PitStopFSM fsm(1, ps);

  fsm.request_stop(p0::race::TireCompound::SOFT, false, true, false);
  fsm.mutable_car_state().assigned_box_id = 0;

  // REQUESTED -> APPROACHING_PIT_LANE
  fsm.update(0.0, 0.0, 0.0);
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::APPROACHING_PIT_LANE);

  // APPROACHING_PIT_LANE -> ENTERING_PIT_LANE
  fsm.update(0.0, 30.0, 5.0);
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::ENTERING_PIT_LANE);

  // ENTERING_PIT_LANE -> PIT_LANE_NAVIGATION (speed <= limit)
  fsm.update(0.0, 50.0, 5.0);
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::PIT_LANE_NAVIGATION);

  // PIT_LANE_NAVIGATION -> BOX_ASSIGNED
  fsm.update(0.0, 50.0, 0.0);
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::BOX_ASSIGNED);

  // BOX_ASSIGNED -> ALIGNING_BOX
  fsm.update(0.0, 50.0, 0.0);
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::ALIGNING_BOX);

  // ALIGNING_BOX -> STOPPED_AT_BOX (car near box at x=50)
  fsm.update(0.0, 50.0, 0.0);
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::STOPPED_AT_BOX);

  // STOPPED_AT_BOX -> RELEASE_AUTHORIZED (service_duration_ = 0)
  fsm.update(0.0, 50.0, 0.0);
  EXPECT_TRUE(fsm.state() >= p0::race::PitStopState::RELEASE_AUTHORIZED);

  // RELEASE_AUTHORIZED -> EXITING_BOX
  fsm.update(0.0, 50.0, 0.0);
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::EXITING_BOX);

  // EXITING_BOX -> PIT_EXIT_NAVIGATION
  fsm.update(0.0, 50.0, 0.0);
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::PIT_EXIT_NAVIGATION);

  // PIT_EXIT_NAVIGATION -> TRACK_REENTRY (car past exit at x=100)
  fsm.update(10.0, 100.0, 5.0);
  EXPECT_EQ(fsm.state(), p0::race::PitStopState::TRACK_REENTRY);

  // TRACK_REENTRY -> COMPLETE
  fsm.update(10.0, 100.0, 5.0);
  EXPECT_TRUE(fsm.is_complete());
  EXPECT_EQ(fsm.car_state().pit_stops_completed, 1);
}

TEST(PitStopFSM, StateNameMapping) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem system(def);
  PitStopFSM fsm(1, system);

  EXPECT_STREQ(fsm.state_name().c_str(), "NONE");

  fsm.request_stop(p0::race::TireCompound::MEDIUM, false, false, false);
  EXPECT_STREQ(fsm.state_name().c_str(), "REQUESTED");
}

TEST(PitStopFSM, DebugStringContainsCarId) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitLaneSystem system(def);
  PitStopFSM fsm(42, system);

  fsm.request_stop(p0::race::TireCompound::MEDIUM, false, false, false);
  std::string dbg = fsm.debug_string();
  EXPECT_NE(dbg.find("car=42"), std::string::npos);
  EXPECT_NE(dbg.find("state=REQUESTED"), std::string::npos);
}

TEST(PitStopManager, RegisterAndQueryCar) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitStopManager mgr(def);

  mgr.register_car(1);
  mgr.register_car(2);

  EXPECT_EQ(mgr.car_state(1), p0::race::PitStopState::NONE);
  EXPECT_EQ(mgr.car_state(2), p0::race::PitStopState::NONE);
  EXPECT_EQ(mgr.car_state(99), p0::race::PitStopState::NONE);
  EXPECT_FALSE(mgr.is_pit_stop_active(1));
}

TEST(PitStopManager, RequestPitStopSuccess) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitStopManager mgr(def);

  mgr.register_car(1);
  bool ok = mgr.request_pit_stop(1, p0::race::TireCompound::SOFT, true, true, false);
  EXPECT_TRUE(ok);
  EXPECT_TRUE(mgr.is_pit_stop_active(1));
  EXPECT_EQ(mgr.car_state(1), p0::race::PitStopState::REQUESTED);
}

TEST(PitStopManager, RequestPitStopForUnregisteredCarFails) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitStopManager mgr(def);

  bool ok = mgr.request_pit_stop(1, p0::race::TireCompound::SOFT, true, true, false);
  EXPECT_FALSE(ok);
}

TEST(PitStopManager, RequestPitStopWhileActiveFails) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitStopManager mgr(def);

  mgr.register_car(1);
  mgr.request_pit_stop(1, p0::race::TireCompound::SOFT, true, true, false);
  bool ok2 = mgr.request_pit_stop(1, p0::race::TireCompound::HARD, false, false, true);
  EXPECT_FALSE(ok2);
}

TEST(PitStopManager, UnregisterRemovesCar) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitStopManager mgr(def);

  mgr.register_car(1);
  mgr.unregister_car(1);
  EXPECT_EQ(mgr.car_state(1), p0::race::PitStopState::NONE);
}

TEST(PitStopManager, ActivePitStopsCount) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitStopManager mgr(def);

  mgr.register_car(1);
  mgr.register_car(2);
  mgr.register_car(3);

  mgr.request_pit_stop(1, p0::race::TireCompound::SOFT, false, false, false);
  mgr.request_pit_stop(2, p0::race::TireCompound::SOFT, false, false, false);

  EXPECT_EQ(mgr.active_pit_stops(), 2);
}

TEST(PitStopManager, PopServedViolations) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  PitStopManager mgr(def);

  mgr.pit_system().on_cross_speed_start(1, 0.0, 0.0);
  mgr.pit_system().on_cross_speed_end(1, 1.0, 20.0);

  auto violations = mgr.pop_served_violations();
  ASSERT_EQ(violations.size(), 1u);
  EXPECT_EQ(violations[0].car_id, 1);
}
