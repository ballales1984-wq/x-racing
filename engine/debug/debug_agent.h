#pragma once

#include "common.h"
#include "simulation/simulation.h"
#include "simulation/simulation_world.h"
#include "telemetry/telemetry.h"
#include "ai/ai_driver.h"
#include "network/network_session.h"
#include "vehicle/car_model.h"
#include "track/track.h"
#include "debug/debug_console.h"
#include "debug/debug_physics.h"
#include "debug/debug_ai.h"
#include "debug/debug_network.h"
#include "debug/debug_profiler.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <mutex>

namespace p0::debug {

enum class DebugSeverity : uint8_t {
  INFO = 0,
  WARN,
  ERR,
  FATAL
};

struct DebugLogEntry {
  double timestamp = 0.0;
  DebugSeverity severity = DebugSeverity::INFO;
  std::string category;
  std::string message;
};

struct DebugSnapshot {
  double sim_time = 0.0;
  double real_time = 0.0;
  int frame_count = 0;

  p0::vehicle::VehicleState state;

  PhysicsDiagnostics physics;
  AIDiagnostics ai;
  NetworkDiagnostics network;
  ProfilerSnapshot profiler;

  std::vector<DebugLogEntry> recent_logs;
  std::string track_info;
  std::string session_info;
};

struct DebugConfig {
  bool enabled = false;
  bool console_enabled = true;
  bool auto_telemetry = false;
  double telemetry_interval_s = 1.0;
  std::string export_directory = "./debug_output";
  int max_log_entries = 1024;
  int max_snapshots = 256;
  bool physics_warnings = true;
  bool ai_warnings = true;
  bool network_warnings = true;
  bool detect_off_track = true;
  bool detect_collision = true;
  bool detect_spin = true;
  double spin_detection_threshold = 3.0;
  double off_track_threshold_s = 2.0;
};

class DebugAgent {
 public:
  DebugAgent();
  ~DebugAgent();

  bool initialize(const DebugConfig& config = {});
  void shutdown();

  void set_simulation(simulation::Simulation* sim);
  void set_simulation_world(simulation::SimulationWorld* world);
  void set_telemetry(telemetry::Telemetry* tel);
  void set_network_session(network::NetworkSession* net);
  void set_track(const track::Track* track);
  void set_local_car_id(int car_id);

  void update(double delta_time, double real_delta_time);
  void record_frame(const p0::vehicle::VehicleState& state, double dt);
  void record_ai_input(int car_id, const input::InputState& input, const ai::AIDriverParams& params);
  void record_network_snapshot(const network::WorldSnapshot& snapshot);

  void toggle_enabled() { config_.enabled = !config_.enabled; }
  void set_enabled(bool e) { config_.enabled = e; }
  bool is_enabled() const { return config_.enabled; }

  const DebugConfig& config() const { return config_; }
  DebugConfig& mutable_config() { return config_; }

  const DebugSnapshot& last_snapshot() const { return last_snapshot_; }
  const std::vector<DebugLogEntry>& logs() const { return logs_; }

  void add_log(DebugSeverity severity, const std::string& category, const std::string& message);

  void export_telemetry_csv(const std::string& path) const;
  void export_snapshot_json(const std::string& path) const;
  void export_logs_csv(const std::string& path) const;

  DebugConsole* console() { return console_; }
  const DebugConsole* console() const { return console_; }

  DebugPhysics* physics() { return physics_; }
  const DebugPhysics* physics() const { return physics_; }

  DebugAI* ai() { return ai_; }
  const DebugAI* ai() const { return ai_; }

  DebugNetwork* network() { return network_; }
  const DebugNetwork* network() const { return network_; }

  DebugProfiler* profiler() { return profiler_; }
  const DebugProfiler* profiler() const { return profiler_; }

  void process_console_command(const std::string& cmd_line);

 private:
  void detect_anomalies(const p0::vehicle::VehicleState& state);
  void update_snapshot(const p0::vehicle::VehicleState& state, double dt);
  void ensure_output_directory() const;

  DebugConfig config_;
  bool initialized_ = false;

  simulation::Simulation* sim_ = nullptr;
  simulation::SimulationWorld* world_ = nullptr;
  telemetry::Telemetry* telemetry_ = nullptr;
  network::NetworkSession* network_session_ = nullptr;
  const track::Track* track_ = nullptr;
  int local_car_id_ = 0;

  DebugConsole* console_ = nullptr;
  DebugPhysics* physics_ = nullptr;
  DebugAI* ai_ = nullptr;
  DebugNetwork* network_ = nullptr;
  DebugProfiler* profiler_ = nullptr;

  DebugSnapshot last_snapshot_;
  std::vector<DebugSnapshot> snapshot_history_;

  std::vector<DebugLogEntry> logs_;
  double last_telemetry_time_ = 0.0;
  double total_sim_time_ = 0.0;
  int frame_count_ = 0;

  double off_track_timer_ = 0.0;
  bool was_off_track_ = false;
  double spin_timer_ = 0.0;
  bool was_spinning_ = false;

  mutable std::mutex mutex_;
};

}
