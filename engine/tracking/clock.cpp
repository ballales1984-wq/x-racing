#include "tracking/clock.h"

#include <chrono>

namespace p0::tracking {

SimulationClock::SimulationClock(double start_time) : time_(start_time) {}

void SimulationClock::set_time(double t) { time_ = t; }

void SimulationClock::advance(double dt) { time_ += dt; }

double SimulationClock::now() const { return time_; }

double RealClock::now() const {
  using namespace std::chrono;
  static const auto epoch = steady_clock::now();
  return duration<double>(steady_clock::now() - epoch).count();
}

ReplayClock::ReplayClock(double start_time, double rate)
    : time_(start_time), rate_(rate) {}

void ReplayClock::set_rate(double rate) { rate_ = rate; }

void ReplayClock::set_time(double t) {
  time_ = t;
  initialized_ = true;
}

void ReplayClock::update() {
  if (!initialized_) {
    last_real_time_ = RealClock{}.now();
    initialized_ = true;
    return;
  }

  const double now = RealClock{}.now();
  const double dt = now - last_real_time_;
  last_real_time_ = now;
  time_ += dt * rate_;
}

double ReplayClock::now() const { return time_; }

}  // namespace p0::tracking
