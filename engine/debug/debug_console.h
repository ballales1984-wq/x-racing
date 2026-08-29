#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace p0::debug {

class DebugAgent;

struct ConsoleCommand {
  std::string name;
  std::string description;
  std::string usage;
  std::function<void(DebugAgent&, const std::vector<std::string>&)> handler;
};

class DebugConsole {
 public:
  DebugConsole() = default;

  void initialize();
  void shutdown();

  void register_command(const ConsoleCommand& cmd);
  void process_command(DebugAgent& agent, const std::string& cmd_line);

  void render_summary(const DebugAgent& agent);

  void print(const std::string& text) const;
  void print_line(const std::string& text = "") const;

  bool is_enabled() const { return enabled_; }
  void set_enabled(bool e) { enabled_ = e; }

  const std::vector<std::string>& history() const { return history_; }
  void clear_history() { history_.clear(); }

 private:
  void register_default_commands();
  std::vector<std::string> tokenize(const std::string& line) const;
  void print_help(const DebugAgent& agent) const;
  void print_help_from_registry() const;
  void print_status(const DebugAgent& agent) const;
  void print_physics(const DebugAgent& agent) const;
  void print_ai(const DebugAgent& agent) const;
  void print_network(const DebugAgent& agent) const;
  void print_profile(const DebugAgent& agent) const;
  void print_track(const DebugAgent& agent) const;
  void cmd_help(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_status(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_physics(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_ai(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_network(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_profile(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_track(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_telemetry(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_export(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_logs(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_set(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_reset(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_respawn(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_toggle(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_clear(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_inspect(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_warnings(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_save(DebugAgent& agent, const std::vector<std::string>& args);
  void cmd_load(DebugAgent& agent, const std::vector<std::string>& args);

  bool enabled_ = true;
  std::vector<ConsoleCommand> commands_;
  std::vector<std::string> history_;
};

}
