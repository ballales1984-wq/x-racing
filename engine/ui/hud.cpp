#include "ui/hud.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace p0::ui {

Hud::Hud() = default;

void Hud::set_config(const HudConfig& config) {
  config_ = config;
}

void Hud::update(const HudState& state) {
  state_ = state;
}

std::string Hud::format_time(double seconds) const {
  if (seconds <= 0.0) return "--:--.---";
  int mins = static_cast<int>(seconds) / 60;
  double secs = seconds - mins * 60;
  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(2) << mins << ":"
      << std::fixed << std::setprecision(3) << std::setw(6) << secs;
  return oss.str();
}

std::string Hud::format_gap(double seconds) const {
  if (seconds <= 0.0) return "---";
  std::ostringstream oss;
  oss << "+" << std::fixed << std::setprecision(3) << seconds;
  return oss.str();
}

}
