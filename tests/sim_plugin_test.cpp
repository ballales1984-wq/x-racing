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

typedef int (*InitFn)();
typedef void (*UpdateFn)(double, double, double, double);
typedef void (*GetStateFn)(PluginVehicleState*);

namespace {
struct Plugin {
  HMODULE dll = nullptr;
  InitFn init = nullptr;
  UpdateFn update = nullptr;
  GetStateFn get_state = nullptr;
  bool ok = false;
};

Plugin& plugin() {
  static Plugin p;
  if (!p.dll) {
    p.dll = LoadLibraryA("D:/x-racing/build/engine/Release/sim_plugin.dll");
    if (p.dll) {
      p.init = reinterpret_cast<InitFn>(GetProcAddress(p.dll, "SimPlugin_Initialize"));
      p.update = reinterpret_cast<UpdateFn>(GetProcAddress(p.dll, "SimPlugin_Update"));
      p.get_state = reinterpret_cast<GetStateFn>(GetProcAddress(p.dll, "SimPlugin_GetVehicleState"));
      p.ok = p.init && p.update && p.get_state;
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
