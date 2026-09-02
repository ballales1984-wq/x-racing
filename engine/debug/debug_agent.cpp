#include "debug/debug_agent.h"
#include "debug/debug_console.h"
#include "debug/debug_physics.h"
#include "debug/debug_ai.h"
#include "debug/debug_network.h"
#include "debug/debug_profiler.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace p0::debug {

//! @brief Default constructor.
DebugAgent::DebugAgent() = default;

//! @brief Destructor. Ensures clean shutdown of debug systems.
DebugAgent::~DebugAgent() {
  shutdown();
}

//! @brief Initializes the debug agent and all subsystems.
//! @param config Debug configuration settings.
//! @return true if initialization succeeded.
bool DebugAgent::initialize(const DebugConfig& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  config_ = config;
  console_ = std::make_unique<DebugConsole>();
  physics_ = std::make_unique<DebugPhysics>();
  ai_ = std::make_unique<DebugAI>();
  network_ = std::make_unique<DebugNetwork>();
  profiler_ = std::make_unique<DebugProfiler>();

  console_->initialize();
  physics_->initialize();
  ai_->initialize();
  network_->initialize();
  profiler_->initialize();
  initialized_ = true;
  add_log(DebugSeverity::INFO, "system", "Debug agent initialized");
  return true;
}

//! @brief Shuts down the debug agent and exports remaining logs.
void DebugAgent::shutdown() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!initialized_) return;

  console_.reset();
  physics_.reset();
  ai_.reset();
  network_.reset();
  profiler_.reset();

  if (!logs_.empty()) {
    export_logs_csv(config_.export_directory + "/debug_log.csv");
  }

  initialized_ = false;
  add_log(DebugSeverity::INFO, "system", "Debug agent shutdown");
}

//! @brief Sets the simulation pointer for state monitoring.
//! @param sim Pointer to the simulation instance.
void DebugAgent::set_simulation(simulation::Simulation* sim) {
  sim_ = sim;
}

//! @brief Sets the simulation world pointer.
//! @param world Pointer to the simulation world.
void DebugAgent::set_simulation_world(simulation::SimulationWorld* world) {
  world_ = world;
}

//! @brief Sets the telemetry interface for auto-export.
//! @param tel Pointer to the telemetry system.
void DebugAgent::set_telemetry(telemetry::Telemetry* tel) {
  telemetry_ = tel;
}

//! @brief Sets the network session for diagnostics.
//! @param net Pointer to the network session.
void DebugAgent::set_network_session(network::NetworkSession* net) {
  network_session_ = net;
}

//! @brief Sets the track pointer for track-related diagnostics.
//! @param track Pointer to the track data.
void DebugAgent::set_track(const track::Track* track) {
  track_ = track;
}

//! @brief Sets the local car ID for focused diagnostics.
//! @param car_id The local car identifier.
void DebugAgent::set_local_car_id(int car_id) {
  local_car_id_ = car_id;
}

//! @brief Main update function called each simulation frame.
//!        Records state, detects anomalies, and updates profiler.
//! @param delta_time Simulation time step.
//! @param real_delta_time Real wall-clock time step.
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

//! @brief Records a frame of simulation data for analysis.
//! @param state Current vehicle state.
//! @param dt Time step.
void DebugAgent::record_frame(const p0::vehicle::VehicleState& state, double dt) {
  physics_->analyze(state, dt);
  profiler_->record_physics_step(dt);

  if (ai_->has_active_driver()) {
    ai_->update_analysis();
  }
}

//! @brief Records AI input for diagnostics.
//! @param car_id The car ID.
//! @param input The AI input state.
//! @param params The AI driver parameters.
void DebugAgent::record_ai_input(int car_id, const input::InputState& input,
                                 const ai::AIDriverParams& params) {
  ai_->record_input(car_id, input, params);
}

//! @brief Records a network snapshot for analysis.
//! @param snapshot The world snapshot from the network.
void DebugAgent::record_network_snapshot(const network::WorldSnapshot& snapshot) {
  network_->analyze_snapshot(snapshot);
}

//! @brief Toggles the debug agent enabled state.
void DebugAgent::toggle_enabled() {
  config_.enabled = !config_.enabled;
  add_log(DebugSeverity::INFO, "system",
          config_.enabled ? "Debug agent enabled" : "Debug agent disabled");
}

//! @brief Adds a log entry to the debug log.
//! @param severity The severity level.
//! @param category The log category.
//! @param message The log message.
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

//! @brief Detects anomalies in the simulation state.
//!        Checks for off-track, spin, and physics warnings.
//! @param state Current vehicle state.
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

//! @brief Updates the debug snapshot with current state.
//! @param state Current vehicle state.
//! @param dt Time step.
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

//! @brief Ensures the output directory exists.
void DebugAgent::ensure_output_directory() const {
  std::filesystem::create_directories(config_.export_directory);
}

//! @brief Exports telemetry data to CSV file.
//! @param path Output file path.
void DebugAgent::export_telemetry_csv(const std::string& path) const {
  if (!telemetry_) return;
  std::lock_guard<std::mutex> lock(mutex_);
  ensure_output_directory();
  telemetry_->save_csv(path);
}

//! @brief Exports the current debug snapshot to JSON file.
//! @param path Output file path.
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

//! @brief Exports all debug logs to CSV file.
//! @param path Output file path.
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

//! @brief Loads a debug snapshot from JSON file.
//! @param path Input file path.
void DebugAgent::load_snapshot_json(const std::string& path) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::ifstream file(path);
  if (!file.is_open()) return;

  std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

  auto find_double = [&](const std::string& key) -> double {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0.0;
    pos = content.find(':', pos);
    if (pos == std::string::npos) return 0.0;
    ++pos;
    while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) ++pos;
    try {
      return std::stod(content.substr(pos));
    } catch (const std::exception&) {
      return 0.0;
    }
  };

  auto find_int = [&](const std::string& key) -> int {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0;
    pos = content.find(':', pos);
    if (pos == std::string::npos) return 0;
    ++pos;
    while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) ++pos;
    try {
      return std::stoi(content.substr(pos));
    } catch (const std::exception&) {
      return 0;
    }
  };

  last_snapshot_.sim_time = find_double("sim_time");
  last_snapshot_.frame_count = find_int("frame");
  last_snapshot_.state.speed = find_double("speed_mps");
  last_snapshot_.state.rpm = find_double("rpm");
  last_snapshot_.state.gear = find_int("gear");
  last_snapshot_.state.lateral_g = find_double("lateral_g");
  last_snapshot_.state.front_tire_temp = find_double("front_tire_temp_k");
  last_snapshot_.state.rear_tire_temp = find_double("rear_tire_temp_k");
  last_snapshot_.state.front_tire_wear = find_double("front_tire_wear");
  last_snapshot_.state.rear_tire_wear = find_double("rear_tire_wear");
  last_snapshot_.state.slip_angle = find_double("slip_angle");
  last_snapshot_.state.slip_ratio = find_double("slip_ratio");
  last_snapshot_.state.aero_drag = find_double("aero_drag_n");
  last_snapshot_.state.aero_downforce = find_double("aero_downforce_n");
  last_snapshot_.profiler.last_frame_time_ms = find_double("frame_time_ms");
  last_snapshot_.profiler.last_physics_time_ms = find_double("physics_time_ms");
}

//! @brief Processes a console command line.
//! @param cmd_line The raw command line string.
void DebugAgent::process_console_command(const std::string& cmd_line) {
  if (console_) console_->process_command(*this, cmd_line);
}

}
