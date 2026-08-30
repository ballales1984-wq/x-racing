// Project 0 — unit tests for the C plugin ABI (sim_plugin)
// Loads the actual sim_plugin.dll and exercises its exported C functions.
#include <gtest/gtest.h>
#include <windows.h>

// Mirror of the C ABI VehicleState struct (must match plugin/sim_plugin.h).
struct PluginVehicleState {
  double x;
  double y;
  double vx;
  double vy;
  double heading;
  double speed;
  double rpm;
  int gear;
  double throttle;
  double brake;
  double steer;
};

// Mirror of the C ABI TrackingSample struct (must match plugin/sim_plugin.h).
struct PluginTrackingSample {
  double latitude;
  double longitude;
  double altitude;
  double track_s;
  double track_l;
  double speed;
  double heading;
  double accuracy;
  int on_track;
};

typedef int (*InitFn)();
typedef void (*UpdateFn)(double, double, double, double);
typedef void (*GetStateFn)(PluginVehicleState*);
typedef int (*GetTrackingFn)(PluginTrackingSample*);

namespace {
struct Plugin {
  HMODULE dll = nullptr;
  InitFn init = nullptr;
  UpdateFn update = nullptr;
  GetStateFn get_state = nullptr;
  GetTrackingFn get_tracking = nullptr;
  bool ok = false;
};

Plugin& plugin() {
  static Plugin p;
  if (!p.dll) {
    p.dll = LoadLibraryA("build/engine/Release/sim_plugin.dll");
    if (p.dll) {
      p.init = reinterpret_cast<InitFn>(GetProcAddress(p.dll, "SimPlugin_Initialize"));
      p.update = reinterpret_cast<UpdateFn>(GetProcAddress(p.dll, "SimPlugin_Update"));
      p.get_state = reinterpret_cast<GetStateFn>(GetProcAddress(p.dll, "SimPlugin_GetVehicleState"));
      p.get_tracking = reinterpret_cast<GetTrackingFn>(GetProcAddress(p.dll, "SimPlugin_GetTracking"));
      p.ok = p.init && p.update && p.get_state && p.get_tracking;
    }
  }
  return p;
}
}  // namespace

// SimPlugin_Initialize returns 0 on success.
TEST(SimPlugin, InitializeReturnsZero) {
  auto& p = plugin();
  if (!p.ok) GTEST_SKIP() << "sim_plugin.dll not built/available";
  EXPECT_EQ(p.init(), 0);
}

// Calling Initialize again is a safe no-op.
TEST(SimPlugin, InitializeIsIdempotent) {
  auto& p = plugin();
  if (!p.ok) GTEST_SKIP() << "sim_plugin.dll not built/available";
  EXPECT_EQ(p.init(), 0);
  EXPECT_EQ(p.init(), 0);
}

// Driving with full throttle should produce forward motion and higher RPM.
TEST(SimPlugin, UpdateMovesVehicle) {
  auto& p = plugin();
  if (!p.ok) GTEST_SKIP() << "sim_plugin.dll not built/available";
  p.init();

  for (int i = 0; i < 200; ++i) {
    p.update(1.0 / 120.0, 1.0, 0.0, 0.0);
  }

  PluginVehicleState state{};
  p.get_state(&state);

  EXPECT_GT(state.speed, 0.0);
  EXPECT_GE(state.rpm, 800.0);
  EXPECT_GT(state.x, 0.0);
}

// Steering should change the vehicle heading.
TEST(SimPlugin, SteeringChangesHeading) {
  auto& p = plugin();
  if (!p.ok) GTEST_SKIP() << "sim_plugin.dll not built/available";
  p.init();

  for (int i = 0; i < 120; ++i) {
    p.update(1.0 / 120.0, 0.5, 0.0, 0.6);
  }

  PluginVehicleState state{};
  p.get_state(&state);
  EXPECT_NE(state.heading, 0.0);
}

// GetVehicleState fills every field of the snapshot struct.
TEST(SimPlugin, GetVehicleStatePopulated) {
  auto& p = plugin();
  if (!p.ok) GTEST_SKIP() << "sim_plugin.dll not built/available";
  p.init();
  p.update(1.0 / 120.0, 1.0, 0.0, 0.0);

  PluginVehicleState state{};
  p.get_state(&state);

  EXPECT_DOUBLE_EQ(state.throttle, 1.0);
  EXPECT_DOUBLE_EQ(state.brake, 0.0);
  EXPECT_DOUBLE_EQ(state.steer, 0.0);
  EXPECT_GE(state.gear, 1);
}

// The tracking layer should expose a valid geographic + track-relative sample.
TEST(SimPlugin, GetTrackingExposed) {
  auto& p = plugin();
  if (!p.ok) GTEST_SKIP() << "sim_plugin.dll not built/available";
  p.init();

  for (int i = 0; i < 200; ++i) {
    p.update(1.0 / 120.0, 1.0, 0.0, 0.0);
  }

  PluginTrackingSample sample{};
  EXPECT_EQ(p.get_tracking(&sample), 0);
  EXPECT_GT(sample.latitude, 0.0);
  EXPECT_GT(sample.longitude, 0.0);
  EXPECT_GE(sample.track_s, 0.0);
}
