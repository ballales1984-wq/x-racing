#include "debug/debug_agent.h"
#include "debug/debug_console.h"
#include "debug/debug_physics.h"
#include "debug/debug_ai.h"
#include "debug/debug_network.h"
#include "debug/debug_profiler.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <sstream>

namespace p0::debug {

DebugAgent::DebugAgent() = default;

DebugAgent::~DebugAgent() {
  shutdown();
}

bool DebugAgent::initialize(const DebugConfig& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  config_ = config;
  console_ = new DebugConsole();
  physics_ = new DebugPhysics();
  ai_ = new DebugAI();
  network_ = new DebugNetwork();
  profiler_ = new DebugProfiler();

  console_->initialize();
  physics_->initialize();
  ai_->initialize();
  network_->initialize();
  profiler_->initialize();
  initialized_ = true;
  add_log(DebugSeverity::INFO, "system", "Debug agent initialized");
  return true;
}

void DebugAgent::shutdown() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!initialized_) return;

  delete console_;
  delete physics_;
  delete ai_;
  delete network_;
  delete profiler_;
  console_ = nullptr;
  physics_ = nullptr;
  ai_ = nullptr;
  network_ = nullptr;
  profiler_ = nullptr;

  if (!logs_.empty()) {
    export_logs_csv(config_.export_directory + "/debug_log.csv");
  }

  initialized_ = false;
  add_log(DebugSeverity::INFO, "system", "Debug agent shutdown");
}

void DebugAgent::set_simulation(simulation::Simulation* sim) {
  sim_ = sim;
}

void DebugAgent::set_simulation_world(simulation::SimulationWorld* world) {
  world_ = world;
}

void DebugAgent::set_telemetry(telemetry::Telemetry* tel) {
  telemetry_ = tel;
}

void DebugAgent::set_network_session(network::NetworkSession* net) {
  network_session_ = net;
}

void DebugAgent::set_track(const track::Track* track) {
  track_ = track;
}

void DebugAgent::set_local_car_id(int car_id) {
  local_car_id_ = car_id;
}

void DebugAgent::update(double delta_time, double real_delta_time) {
  if (!config_.enabled || !initialized_) return;

  profiler_->begin_frame(real_delta_time);
  total_sim_time_ += delta_time;
  frame_count_++;

  if (sim_) {
    const auto& state = sim_->state();
    record_frame(state, delta_time);
    detect_anomalies(state);
    update_snapshot(state, delta_time);
  }

  if (config_.auto_telemetry &&
      total_sim_time_ - last_telemetry_time_ >= config_.telemetry_interval_s) {
    last_telemetry_time_ = total_sim_time_;
    if (telemetry_) {
      export_telemetry_csv(config_.export_directory + "/auto_telemetry.csv");
    }
  }

  profiler_->end_frame();

  if (frame_count_ % 60 == 0 && config_.console_enabled) {
    console_->render_summary(*this);
  }
}

void DebugAgent::record_frame(const p0::vehicle::VehicleState& state, double dt) {
  physics_->analyze(state, dt);
  profiler_->record_physics_step(dt);

  if (ai_->has_active_driver()) {
    ai_->update_analysis();
  }
}

void DebugAgent::record_ai_input(int car_id, const input::InputState& input,
                                 const ai::AIDriverParams& params) {
  ai_->record_input(car_id, input, params);
}

void DebugAgent::record_network_snapshot(const network::WorldSnapshot& snapshot) {
  network_->analyze_snapshot(snapshot);
}

void DebugAgent::toggle_enabled() {
  config_.enabled = !config_.enabled;
  add_log(DebugSeverity::INFO, "system",
          config_.enabled ? "Debug agent enabled" : "Debug agent disabled");
}

void DebugAgent::add_log(DebugSeverity severity, const std::string& category,
                         const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  DebugLogEntry entry;
  entry.timestamp = total_sim_time_;
  entry.severity = severity;
  entry.category = category;
  entry.message = message;
  logs_.push_back(entry);

  if (static_cast<int>(logs_.size()) > config_.max_log_entries) {
    logs_.erase(logs_.begin(), logs_.begin() + (logs_.size() - config_.max_log_entries));
  }

  if (severity >= DebugSeverity::FATAL) {
    std::fprintf(stderr, "[DEBUG:%s] %s\n", category.c_str(), message.c_str());
  }
}

void DebugAgent::detect_anomalies(const p0::vehicle::VehicleState& state) {
  if (config_.detect_off_track && track_) {
    const bool currently_off = std::abs(state.lateral_velocity) > 5.0 && state.speed < 2.0;
    if (currently_off && !was_off_track_) {
      off_track_timer_ = 0.0;
    }
    if (currently_off) {
      off_track_timer_ += 1.0 / 60.0;
      if (off_track_timer_ >= config_.off_track_threshold_s) {
        add_log(DebugSeverity::WARN, "track",
                "Car off-track for " + std::to_string(static_cast<int>(off_track_timer_)) + "s");
        off_track_timer_ = 0.0;
      }
    }
    was_off_track_ = currently_off;
  }

  if (config_.detect_spin) {
    const bool spinning = std::abs(state.yaw_rate) > config_.spin_detection_threshold &&
                          state.speed > 5.0;
    if (spinning && !was_spinning_) {
      spin_timer_ = 0.0;
    }
    if (spinning) {
      spin_timer_ += 1.0 / 60.0;
      if (spin_timer_ >= 0.5) {
        add_log(DebugSeverity::WARN, "physics",
                "Spin detected: yaw_rate=" + std::to_string(state.yaw_rate) + " rad/s");
        spin_timer_ = 0.0;
      }
    }
    was_spinning_ = spinning;
  }

  if (config_.physics_warnings) {
    if (state.front_tire_temp < 280.0 || state.front_tire_temp > 380.0) {
      add_log(DebugSeverity::WARN, "physics",
              "Front tire temp out of range: " + std::to_string(state.front_tire_temp) + "K");
    }
    if (state.rear_tire_temp < 280.0 || state.rear_tire_temp > 380.0) {
      add_log(DebugSeverity::WARN, "physics",
              "Rear tire temp out of range: " + std::to_string(state.rear_tire_temp) + "K");
    }
    if (state.speed > 100.0) {
      add_log(DebugSeverity::INFO, "physics",
              "High speed: " + std::to_string(static_cast<int>(state.speed * 3.6)) + " km/h");
    }
  }
}

void DebugAgent::update_snapshot(const p0::vehicle::VehicleState& state, double dt) {
  std::lock_guard<std::mutex> lock(mutex_);
  last_snapshot_.sim_time = total_sim_time_;
  last_snapshot_.frame_count = frame_count_;

  auto now = std::chrono::high_resolution_clock::now();
  static auto start_time = now;
  last_snapshot_.real_time =
      std::chrono::duration<double>(now - start_time).count();

  last_snapshot_.state = state;
  last_snapshot_.physics = physics_->current_diagnostics();
  last_snapshot_.ai = ai_->current_diagnostics();
  last_snapshot_.network = network_->current_diagnostics();
  last_snapshot_.profiler = profiler_->snapshot();

  if (!last_snapshot_.recent_logs.empty()) {
    last_snapshot_.recent_logs.clear();
  }
  const int log_start = std::max(0, static_cast<int>(logs_.size()) - 32);
  for (int i = log_start; i < static_cast<int>(logs_.size()); ++i) {
    last_snapshot_.recent_logs.push_back(logs_[i]);
  }

  if (track_) {
    std::ostringstream ss;
    ss << "Track: " << track_->length() << "m, type="
       << static_cast<int>(track_->track_type());
    last_snapshot_.track_info = ss.str();
  }

  std::ostringstream ss2;
  ss2 << "Car " << local_car_id_ << " | Lap " << state.lap
      << " | Speed " << static_cast<int>(state.speed * 3.6) << " km/h"
      << " | Gear " << state.gear << " | RPM " << static_cast<int>(state.rpm);
  last_snapshot_.session_info = ss2.str();

  snapshot_history_.push_back(last_snapshot_);
  if (static_cast<int>(snapshot_history_.size()) > config_.max_snapshots) {
    snapshot_history_.erase(snapshot_history_.begin(),
                            snapshot_history_.begin() + (snapshot_history_.size() - config_.max_snapshots));
  }
}

void DebugAgent::ensure_output_directory() const {
  std::filesystem::create_directories(config_.export_directory);
}

void DebugAgent::export_telemetry_csv(const std::string& path) const {
  if (!telemetry_) return;
  std::lock_guard<std::mutex> lock(mutex_);
  ensure_output_directory();
  telemetry_->save_csv(path);
}

void DebugAgent::export_snapshot_json(const std::string& path) const {
  std::lock_guard<std::mutex> lock(mutex_);
  ensure_output_directory();
  std::ofstream file(path);
  if (!file.is_open()) return;

  file << "{\n";
  file << "  \"sim_time\": " << last_snapshot_.sim_time << ",\n";
  file << "  \"frame\": " << last_snapshot_.frame_count << ",\n";
  file << "  \"speed_mps\": " << last_snapshot_.state.speed << ",\n";
  file << "  \"speed_kph\": " << static_cast<int>(last_snapshot_.state.speed * 3.6) << ",\n";
  file << "  \"rpm\": " << last_snapshot_.state.rpm << ",\n";
  file << "  \"gear\": " << last_snapshot_.state.gear << ",\n";
  file << "  \"lateral_g\": " << last_snapshot_.state.lateral_g << ",\n";
  file << "  \"front_tire_temp_k\": " << last_snapshot_.state.front_tire_temp << ",\n";
  file << "  \"rear_tire_temp_k\": " << last_snapshot_.state.rear_tire_temp << ",\n";
  file << "  \"front_tire_wear\": " << last_snapshot_.state.front_tire_wear << ",\n";
  file << "  \"rear_tire_wear\": " << last_snapshot_.state.rear_tire_wear << ",\n";
  file << "  \"slip_angle\": " << last_snapshot_.state.slip_angle << ",\n";
  file << "  \"slip_ratio\": " << last_snapshot_.state.slip_ratio << ",\n";
  file << "  \"aero_drag_n\": " << last_snapshot_.state.aero_drag << ",\n";
  file << "  \"aero_downforce_n\": " << last_snapshot_.state.aero_downforce << ",\n";
  file << "  \"frame_time_ms\": " << last_snapshot_.profiler.last_frame_time_ms << ",\n";
  file << "  \"physics_time_ms\": " << last_snapshot_.profiler.last_physics_time_ms << "\n";
  file << "}\n";
}

void DebugAgent::export_logs_csv(const std::string& path) const {
  std::lock_guard<std::mutex> lock(mutex_);
  ensure_output_directory();
  std::ofstream file(path);
  if (!file.is_open()) return;

  file << "time,severity,category,message\n";
  for (const auto& entry : logs_) {
    const char* sev = "INFO";
    switch (entry.severity) {
      case DebugSeverity::WARN: sev = "WARN"; break;
      case DebugSeverity::ERR: sev = "ERROR"; break;
      case DebugSeverity::FATAL: sev = "FATAL"; break;
      default: break;
    }
    file << entry.timestamp << "," << sev << ",\""
         << entry.category << "\",\""
         << entry.message << "\"\n";
  }
}

void DebugAgent::process_console_command(const std::string& cmd_line) {
  if (console_) console_->process_command(*this, cmd_line);
}

}
