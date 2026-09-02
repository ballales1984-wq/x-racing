#include "debug/debug_console.h"
#include "debug/debug_agent.h"
#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <sstream>

namespace p0::debug {

//! @brief Initializes the debug console and registers default commands.
void DebugConsole::initialize() {
  register_default_commands();
}

//! @brief Shuts down the console, clearing commands and history.
void DebugConsole::shutdown() {
  commands_.clear();
  history_.clear();
}

//! @brief Registers all built-in debug commands.
void DebugConsole::register_default_commands() {
  commands_.clear();

  commands_.push_back({"help", "Show available commands", "help",
    [](DebugAgent&, const std::vector<std::string>&) {}});

  commands_.push_back({"status", "Show debug agent status", "status",
    [this](DebugAgent& a, const std::vector<std::string>&) { print_status(a); }});

  commands_.push_back({"physics", "Show physics diagnostics", "physics",
    [this](DebugAgent& a, const std::vector<std::string>&) { print_physics(a); }});

  commands_.push_back({"ai", "Show AI diagnostics", "ai",
    [this](DebugAgent& a, const std::vector<std::string>&) { print_ai(a); }});

  commands_.push_back({"network", "Show network diagnostics", "network",
    [this](DebugAgent& a, const std::vector<std::string>&) { print_network(a); }});

  commands_.push_back({"profile", "Show performance profile", "profile",
    [this](DebugAgent& a, const std::vector<std::string>&) { print_profile(a); }});

  commands_.push_back({"track", "Show track info", "track",
    [this](DebugAgent& a, const std::vector<std::string>&) { print_track(a); }});

  commands_.push_back({"telemetry", "Export telemetry to CSV", "telemetry [path]",
    [this](DebugAgent& a, const std::vector<std::string>& args) { cmd_telemetry(a, args); }});

  commands_.push_back({"export", "Export debug snapshot to JSON", "export [path]",
    [this](DebugAgent& a, const std::vector<std::string>& args) { cmd_export(a, args); }});

  commands_.push_back({"logs", "Show recent debug logs", "logs [count]",
    [this](DebugAgent& a, const std::vector<std::string>& args) { cmd_logs(a, args); }});

  commands_.push_back({"set", "Set a debug config value", "set <key> <value>",
    [this](DebugAgent& a, const std::vector<std::string>& args) { cmd_set(a, args); }});

  commands_.push_back({"reset", "Reset simulation state", "reset",
    [this](DebugAgent& a, const std::vector<std::string>&) { cmd_reset(a, {}); }});

  commands_.push_back({"respawn", "Respawn the local car", "respawn",
    [this](DebugAgent& a, const std::vector<std::string>&) { cmd_respawn(a, {}); }});

  commands_.push_back({"toggle", "Toggle debug on/off", "toggle",
    [this](DebugAgent& a, const std::vector<std::string>&) { cmd_toggle(a, {}); }});

  commands_.push_back({"clear", "Clear console history", "clear",
    [this](DebugAgent& a, const std::vector<std::string>&) { cmd_clear(a, {}); }});

  commands_.push_back({"inspect", "Inspect a specific value", "inspect <field>",
    [this](DebugAgent& a, const std::vector<std::string>& args) { cmd_inspect(a, args); }});

  commands_.push_back({"warnings", "Toggle warning detectors", "warnings <on|off> [type]",
    [this](DebugAgent& a, const std::vector<std::string>& args) { cmd_warnings(a, args); }});

  commands_.push_back({"save", "Save debug state", "save [path]",
    [this](DebugAgent& a, const std::vector<std::string>& args) { cmd_save(a, args); }});

  commands_.push_back({"load", "Load debug state", "load [path]",
    [this](DebugAgent& a, const std::vector<std::string>& args) { cmd_load(a, args); }});
}

//! @brief Registers a custom console command.
//! @param cmd The command to register.
void DebugConsole::register_command(const ConsoleCommand& cmd) {
  commands_.push_back(cmd);
}

//! @brief Processes a command line input and executes the matching command.
//! @param agent Reference to the debug agent.
//! @param cmd_line The raw command line string.
void DebugConsole::process_command(DebugAgent& agent, const std::string& cmd_line) {
  auto tokens = tokenize(cmd_line);
  if (tokens.empty()) return;

  history_.push_back(cmd_line);
  if (history_.size() > 256) history_.erase(history_.begin());

  const std::string& cmd = tokens[0];
  for (const auto& command : commands_) {
    if (command.name == cmd) {
      std::vector<std::string> args(tokens.begin() + 1, tokens.end());
      command.handler(agent, args);
      return;
    }
  }

  print_line("Unknown command: " + cmd + " (type 'help' for available commands)");
}

//! @brief Renders a one-line debug summary to stdout.
//! @param agent Reference to the debug agent.
void DebugConsole::render_summary(const DebugAgent& agent) {
  if (!enabled_) return;
  const auto& snap = agent.last_snapshot();
  std::printf("[DEBUG] F:%06d T:%.1fs | %s | %.0fkm/h G%d R%d | phys:%.2fms\n",
              snap.frame_count, snap.sim_time,
              snap.session_info.c_str(),
              snap.state.speed * 3.6, snap.state.gear, static_cast<int>(snap.state.rpm),
              snap.profiler.last_physics_time_ms);
}

//! @brief Prints text to stdout.
//! @param text The text to print.
void DebugConsole::print(const std::string& text) const {
  std::fputs(text.c_str(), stdout);
}

//! @brief Prints a line of text to stdout (with newline).
//! @param text The text to print.
void DebugConsole::print_line(const std::string& text) const {
  print(text + "\n");
}

//! @brief Tokenizes a string by whitespace.
//! @param line The input string.
//! @return Vector of tokens.
std::vector<std::string> DebugConsole::tokenize(const std::string& line) const {
  std::vector<std::string> tokens;
  std::string current;
  for (char c : line) {
    if (c == ' ' || c == '\t') {
      if (!current.empty()) {
        tokens.push_back(current);
        current.clear();
      }
    } else {
      current += c;
    }
  }
  if (!current.empty()) tokens.push_back(current);
  return tokens;
}

//! @brief Prints help for all registered commands.
void DebugConsole::print_help_from_registry() const {
  print_line("=== Debug Console Commands ===");
  for (const auto& cmd : commands_) {
    std::printf("  %-12s %s\n", cmd.name.c_str(), cmd.description.c_str());
  }
  print_line("Usage: <command> [args]");
}

//! @brief Prints help (wrapper for print_help_from_registry).
//! @param agent Reference to the debug agent (unused).
void DebugConsole::print_help(const DebugAgent&) const {
  print_help_from_registry();
}

//! @brief Prints debug agent status information.
//! @param agent Reference to the debug agent.
void DebugConsole::print_status(const DebugAgent& agent) const {
  const auto& cfg = agent.config();
  const auto& snap = agent.last_snapshot();
  print_line("=== Debug Agent Status ===");
  std::printf("  Enabled:       %s\n", cfg.enabled ? "yes" : "no");
  std::printf("  Console:       %s\n", cfg.console_enabled ? "yes" : "no");
  std::printf("  Auto telemetry:%s\n", cfg.auto_telemetry ? "yes" : "no");
  std::printf("  Sim time:      %.2fs\n", snap.sim_time);
  std::printf("  Frame:         %d\n", snap.frame_count);
  std::printf("  Logs:          %zu entries\n", agent.logs().size());
}

//! @brief Prints physics diagnostics.
//! @param agent Reference to the debug agent.
void DebugConsole::print_physics(const DebugAgent& agent) const {
  const auto& state = agent.last_snapshot().state;
  print_line("=== Physics Diagnostics ===");
  std::printf("  Speed:         %.1f m/s (%.0f km/h)\n", state.speed, state.speed * 3.6);
  std::printf("  Heading:       %.2f rad\n", state.heading);
  std::printf("  Yaw rate:      %.3f rad/s\n", state.yaw_rate);
  std::printf("  Steer angle:   %.3f rad\n", state.steer_angle);
  std::printf("  Throttle:      %.0f%%\n", state.throttle * 100.0);
  std::printf("  Brake:         %.0f%%\n", state.brake * 100.0);
  std::printf("  RPM:           %.0f\n", state.rpm);
  std::printf("  Gear:          %d\n", state.gear);
  std::printf("  Slip angle:    %.4f rad\n", state.slip_angle);
  std::printf("  Slip ratio:    %.4f\n", state.slip_ratio);
  std::printf("  Front tire temp: %.0f K (%.0f C)\n",
              state.front_tire_temp, state.front_tire_temp - 273.15);
  std::printf("  Rear tire temp:  %.0f K (%.0f C)\n",
              state.rear_tire_temp, state.rear_tire_temp - 273.15);
  std::printf("  Front tire wear: %.1f%%\n", (1.0 - state.front_tire_wear) * 100.0);
  std::printf("  Rear tire wear:  %.1f%%\n", (1.0 - state.rear_tire_wear) * 100.0);
  std::printf("  Aero drag:     %.0f N\n", state.aero_drag);
  std::printf("  Aero downforce:%.0f N\n", state.aero_downforce);
  std::printf("  Lateral G:     %.2f g\n", state.lateral_g);
  std::printf("  FL load:       %.0f N\n", state.fl_tire_load);
  std::printf("  FR load:       %.0f N\n", state.fr_tire_load);
  std::printf("  RL load:       %.0f N\n", state.rl_tire_load);
  std::printf("  RR load:       %.0f N\n", state.rr_tire_load);
  std::printf("  Body roll:     %.3f rad\n", state.body_roll);
  std::printf("  Body pitch:    %.3f rad\n", state.body_pitch);
  std::printf("  Engine torque: %.0f Nm\n", state.engine_torque);
}

//! @brief Prints AI diagnostics.
//! @param agent Reference to the debug agent.
void DebugConsole::print_ai(const DebugAgent& agent) const {
  const auto& ai_diag = agent.ai()->current_diagnostics();
  print_line("=== AI Diagnostics ===");
  std::printf("  Active drivers: %d\n", ai_diag.active_driver_count);
  for (const auto& pr : ai_diag.driver_info) {
    const auto& info = pr.second;
    std::printf("  Car %d: target_speed=%.0f km/h steer=%.2f throttle=%.0f%% brake=%.0f%%\n",
                info.car_id,
                info.target_speed * 3.6,
                info.steer_input,
                info.throttle_input * 100.0,
                info.brake_input * 100.0);
    std::printf("         lookahead=%.0f diff=%.0f km/h\n",
                info.lookahead_distance,
                (info.target_speed - info.current_speed) * 3.6);
  }
}

//! @brief Prints network diagnostics.
//! @param agent Reference to the debug agent.
void DebugConsole::print_network(const DebugAgent& agent) const {
  const auto& net = agent.network()->current_diagnostics();
  print_line("=== Network Diagnostics ===");
  std::printf("  Connected:     %s\n", net.connected ? "yes" : "no");
  std::printf("  Role:          %s\n", net.is_host ? "HOST" : "CLIENT");
  std::printf("  Players:       %d\n", net.player_count);
  std::printf("  Packets sent:  %d\n", net.packets_sent);
  std::printf("  Packets recv:  %d\n", net.packets_received);
  std::printf("  Packet loss:   %.1f%%\n", net.packet_loss_percent);
  std::printf("  Bytes sent:    %.0f\n", net.total_bytes_sent);
  std::printf("  Bytes recv:    %.0f\n", net.total_bytes_received);
  std::printf("  State:         %d\n", static_cast<int>(net.connection_state));
}

//! @brief Prints performance profiling information.
//! @param agent Reference to the debug agent.
void DebugConsole::print_profile(const DebugAgent& agent) const {
  const auto& prof = agent.profiler()->snapshot();
  print_line("=== Performance Profile ===");
  std::printf("  FPS:           %.1f\n", prof.fps);
  std::printf("  Frame time:    %.2f ms\n", prof.last_frame_time_ms);
  std::printf("  Physics time:  %.2f ms\n", prof.last_physics_time_ms);
  std::printf("  Avg frame:     %.2f ms\n", prof.avg_frame_time_ms);
  std::printf("  Max frame:     %.2f ms\n", prof.max_frame_time_ms);
  std::printf("  Min frame:     %.2f ms\n", prof.min_frame_time_ms);
  std::printf("  Total frames:  %d\n", prof.total_frames);
  std::printf("  Dropped:       %d\n", prof.dropped_frames);
  std::printf("  Physics ratio: %.1f%%\n", prof.physics_ratio * 100.0);
}

//! @brief Prints track information.
//! @param agent Reference to the debug agent.
void DebugConsole::print_track(const DebugAgent& agent) const {
  const auto& snap = agent.last_snapshot();
  print_line("=== Track Info ===");
  std::printf("  %s\n", snap.track_info.c_str());
  std::printf("  %s\n", snap.session_info.c_str());
}

//! @brief Command: Shows help.
void DebugConsole::cmd_help(DebugAgent&, const std::vector<std::string>&) {
  print_help_from_registry();
}

//! @brief Command: Shows debug agent status.
void DebugConsole::cmd_status(DebugAgent& agent, const std::vector<std::string>&) {
  print_status(agent);
}

//! @brief Command: Shows physics diagnostics.
void DebugConsole::cmd_physics(DebugAgent& agent, const std::vector<std::string>&) {
  print_physics(agent);
}

//! @brief Command: Shows AI diagnostics.
void DebugConsole::cmd_ai(DebugAgent& agent, const std::vector<std::string>&) {
  print_ai(agent);
}

//! @brief Command: Shows network diagnostics.
void DebugConsole::cmd_network(DebugAgent& agent, const std::vector<std::string>&) {
  print_network(agent);
}

//! @brief Command: Shows performance profile.
void DebugConsole::cmd_profile(DebugAgent& agent, const std::vector<std::string>&) {
  print_profile(agent);
}

//! @brief Command: Shows track info.
void DebugConsole::cmd_track(DebugAgent& agent, const std::vector<std::string>&) {
  print_track(agent);
}

//! @brief Command: Exports telemetry to CSV file.
//! @param agent Reference to the debug agent.
//! @param args Command arguments (optional file path).
void DebugConsole::cmd_telemetry(DebugAgent& agent, const std::vector<std::string>& args) {
  const std::string path = args.empty() ? "debug_output/telemetry.csv" : args[0];
  agent.export_telemetry_csv(path);
  print_line("Telemetry exported to: " + path);
}

//! @brief Command: Exports debug snapshot to JSON file.
//! @param agent Reference to the debug agent.
//! @param args Command arguments (optional file path).
void DebugConsole::cmd_export(DebugAgent& agent, const std::vector<std::string>& args) {
  const std::string path = args.empty() ? "debug_output/snapshot.json" : args[0];
  agent.export_snapshot_json(path);
  print_line("Snapshot exported to: " + path);
}

//! @brief Command: Shows recent debug logs.
//! @param agent Reference to the debug agent.
//! @param args Command arguments (optional count).
void DebugConsole::cmd_logs(DebugAgent& agent, const std::vector<std::string>& args) {
  int count = 20;
  if (!args.empty()) count = std::stoi(args[0]);
  const auto& logs = agent.logs();
  const int start = std::max(0, static_cast<int>(logs.size()) - count);
  print_line("=== Recent Logs ===");
  for (int i = start; i < static_cast<int>(logs.size()); ++i) {
    const auto& entry = logs[i];
    const char* sev = "INFO";
    switch (entry.severity) {
      case DebugSeverity::WARN: sev = "WARN"; break;
      case DebugSeverity::ERR: sev = "ERROR"; break;
      case DebugSeverity::FATAL: sev = "FATAL"; break;
      default: break;
    }
    std::printf("  [%.1fs][%s][%s] %s\n",
                entry.timestamp, sev, entry.category.c_str(), entry.message.c_str());
  }
}

//! @brief Command: Sets a debug configuration value.
//! @param agent Reference to the debug agent.
//! @param args Command arguments (key, value).
void DebugConsole::cmd_set(DebugAgent& agent, const std::vector<std::string>& args) {
  if (args.size() < 2) {
    print_line("Usage: set <key> <value>");
    return;
  }
  auto& cfg = agent.mutable_config();
  const std::string& key = args[0];
  const std::string& val = args[1];

  if (key == "enabled") cfg.enabled = (val == "1" || val == "true" || val == "on");
  else if (key == "console") cfg.console_enabled = (val == "1" || val == "true" || val == "on");
  else if (key == "auto_telemetry") cfg.auto_telemetry = (val == "1" || val == "true" || val == "on");
  else if (key == "physics_warnings") cfg.physics_warnings = (val == "1" || val == "true" || val == "on");
  else if (key == "ai_warnings") cfg.ai_warnings = (val == "1" || val == "true" || val == "on");
  else if (key == "network_warnings") cfg.network_warnings = (val == "1" || val == "true" || val == "on");
  else if (key == "detect_off_track") cfg.detect_off_track = (val == "1" || val == "true" || val == "on");
  else if (key == "detect_collision") cfg.detect_collision = (val == "1" || val == "true" || val == "on");
  else if (key == "detect_spin") cfg.detect_spin = (val == "1" || val == "true" || val == "on");
  else if (key == "spin_threshold") cfg.spin_detection_threshold = std::stod(val);
  else if (key == "offtrack_threshold") cfg.off_track_threshold_s = std::stod(val);
  else if (key == "telemetry_interval") cfg.telemetry_interval_s = std::stod(val);
  else {
    print_line("Unknown config key: " + key);
    return;
  }
  print_line("Set " + key + " = " + val);
}

//! @brief Command: Requests simulation reset.
void DebugConsole::cmd_reset(DebugAgent&, const std::vector<std::string>&) {
  print_line("Reset requested");
}

//! @brief Command: Requests car respawn.
void DebugConsole::cmd_respawn(DebugAgent&, const std::vector<std::string>&) {
  print_line("Respawn requested");
}

//! @brief Command: Toggles debug agent enabled state.
//! @param agent Reference to the debug agent.
void DebugConsole::cmd_toggle(DebugAgent& agent, const std::vector<std::string>&) {
  agent.toggle_enabled();
  print_line("Debug agent: " + std::string(agent.is_enabled() ? "enabled" : "disabled"));
}

//! @brief Command: Clears console history.
void DebugConsole::cmd_clear(DebugAgent&, const std::vector<std::string>&) {
  history_.clear();
  print_line("Console history cleared");
}

//! @brief Command: Inspects a specific debug value by field name.
//! @param agent Reference to the debug agent.
//! @param args Command arguments (field name).
void DebugConsole::cmd_inspect(DebugAgent& agent, const std::vector<std::string>& args) {
  if (args.empty()) {
    print_line("Usage: inspect <field>");
    return;
  }
  const std::string& field = args[0];
  const auto& state = agent.last_snapshot().state;
  if (field == "speed") std::printf("speed = %.2f m/s\n", state.speed);
  else if (field == "rpm") std::printf("rpm = %.0f\n", state.rpm);
  else if (field == "gear") std::printf("gear = %d\n", state.gear);
  else if (field == "position") std::printf("position = (%.2f, %.2f)\n", state.position.x(), state.position.y());
  else if (field == "heading") std::printf("heading = %.4f rad\n", state.heading);
  else if (field == "slip_angle") std::printf("slip_angle = %.4f rad\n", state.slip_angle);
  else if (field == "slip_ratio") std::printf("slip_ratio = %.4f\n", state.slip_ratio);
  else if (field == "lateral_g") std::printf("lateral_g = %.2f g\n", state.lateral_g);
  else if (field == "tire_temp_front") std::printf("front_tire_temp = %.0f K\n", state.front_tire_temp);
  else if (field == "tire_temp_rear") std::printf("rear_tire_temp = %.0f K\n", state.rear_tire_temp);
  else if (field == "tire_wear_front") std::printf("front_tire_wear = %.1f%%\n", (1.0 - state.front_tire_wear) * 100.0);
  else if (field == "tire_wear_rear") std::printf("rear_tire_wear = %.1f%%\n", (1.0 - state.rear_tire_wear) * 100.0);
  else if (field == "aero_drag") std::printf("aero_drag = %.0f N\n", state.aero_drag);
  else if (field == "aero_downforce") std::printf("aero_downforce = %.0f N\n", state.aero_downforce);
  else if (field == "yaw_rate") std::printf("yaw_rate = %.3f rad/s\n", state.yaw_rate);
  else if (field == "steer") std::printf("steer_angle = %.3f rad\n", state.steer_angle);
  else print_line("Unknown field: " + field);
}

//! @brief Command: Toggles warning detectors on/off.
//! @param agent Reference to the debug agent.
//! @param args Command arguments (on/off, optional type).
void DebugConsole::cmd_warnings(DebugAgent& agent, const std::vector<std::string>& args) {
  if (args.empty()) {
    print_line("Usage: warnings <on|off> [physics|ai|network|offtrack|collision|spin]");
    return;
  }
  const bool on = (args[0] == "on" || args[0] == "1" || args[0] == "true");
  auto& cfg = agent.mutable_config();
  if (args.size() >= 2) {
    const std::string& type = args[1];
    if (type == "physics") cfg.physics_warnings = on;
    else if (type == "ai") cfg.ai_warnings = on;
    else if (type == "network") cfg.network_warnings = on;
    else if (type == "offtrack") cfg.detect_off_track = on;
    else if (type == "collision") cfg.detect_collision = on;
    else if (type == "spin") cfg.detect_spin = on;
    else { print_line("Unknown warning type: " + type); return; }
  } else {
    cfg.physics_warnings = on;
    cfg.ai_warnings = on;
    cfg.network_warnings = on;
    cfg.detect_off_track = on;
    cfg.detect_collision = on;
    cfg.detect_spin = on;
  }
  print_line("Warnings " + std::string(on ? "enabled" : "disabled"));
}

//! @brief Command: Saves debug state to file.
//! @param agent Reference to the debug agent.
//! @param args Command arguments (optional file path).
void DebugConsole::cmd_save(DebugAgent& agent, const std::vector<std::string>& args) {
  const std::string path = args.empty() ? "debug_output/debug_state.json" : args[0];
  agent.export_snapshot_json(path);
  agent.export_logs_csv("debug_output/debug_log.csv");
  print_line("Debug state saved to: " + path);
}

//! @brief Command: Loads debug state from file.
//! @param agent Reference to the debug agent.
//! @param args Command arguments (file path).
void DebugConsole::cmd_load(DebugAgent& agent, const std::vector<std::string>& args) {
  if (args.empty()) {
    print_line("Usage: load <path>");
    return;
  }
  agent.load_snapshot_json(args[0]);
  print_line("Debug state loaded from: " + args[0]);
}

}