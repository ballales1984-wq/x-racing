#include "ui/hud.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace p0::ui {

//! @brief Default constructor.
Hud::Hud() = default;

//! @brief Sets the HUD configuration.
//! @param config The HUD display configuration.
void Hud::set_config(const HudConfig& config) {
  config_ = config;
}

//! @brief Updates the HUD state with current race data.
//! @param state The new HUD state.
void Hud::update(const HudState& state) {
  state_ = state;
}

//! @brief Formats a time value as MM:SS.mmm string.
//! @param seconds Time in seconds.
//! @return Formatted time string, or "--:---.---" if invalid.
std::string Hud::format_time(double seconds) const {
  if (seconds <= 0.0) return "--:--.---";
  int mins = static_cast<int>(seconds) / 60;
  double secs = seconds - mins * 60;
  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(2) << mins << ":"
      << std::fixed << std::setprecision(3) << std::setw(6) << secs;
  return oss.str();
}

//! @brief Formats a gap time as +SS.mmm string.
//! @param seconds Gap time in seconds.
//! @return Formatted gap string, or "---" if invalid.
std::string Hud::format_gap(double seconds) const {
  if (seconds <= 0.0) return "---";
  std::ostringstream oss;
  oss << "+" << std::fixed << std::setprecision(3) << seconds;
  return oss.str();
}

}