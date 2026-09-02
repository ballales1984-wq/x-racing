#include "tracking/lap_system.h"

namespace p0::tracking {

LapSystem::LapSystem(double track_length, int total_laps)
    : lap_detector_(track_length, total_laps) {}

void LapSystem::update(const TrackPosition& pos, double timestamp) {
  if (!lap_started_) {
    lap_started_ = true;
    lap_start_time_ = timestamp;
    emit_event(LapEvent::Type::kLapStarted, timestamp);
  }

  double wrapped_s = pos.s;
  const double track_len = lap_detector_.track_length();
  if (track_len > kEpsilon) {
    wrapped_s = std::fmod(wrapped_s, track_len);
    if (wrapped_s < 0.0) wrapped_s += track_len;
  }

  const bool crossed = lap_detector_.update(wrapped_s);
  current_lap_time_ = timestamp - lap_start_time_;

  if (crossed) {
    lap_finished_ = true;
    last_lap_time_ = current_lap_time_;
    ++lap_number_;
    emit_event(LapEvent::Type::kLapFinished, timestamp);
  }
}

void LapSystem::reset() {
  lap_detector_.reset();
  lap_started_ = false;
  lap_finished_ = false;
  lap_invalidated_ = false;
  lap_start_time_ = 0.0;
  current_lap_time_ = 0.0;
  last_lap_time_ = 0.0;
  lap_number_ = 0;
}

std::vector<LapEvent> LapSystem::pop_events() {
  std::vector<LapEvent> result = std::move(events_);
  events_.clear();
  return result;
}

void LapSystem::emit_event(LapEvent::Type type, double timestamp) {
  LapEvent ev{};
  ev.type = type;
  ev.timestamp = timestamp;
  ev.lap_time = current_lap_time_;
  ev.lap_number = lap_number_;
  events_.push_back(ev);
}

}  // namespace p0::tracking
