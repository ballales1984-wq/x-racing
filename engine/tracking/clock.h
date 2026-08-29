#pragma once

#include "common.h"

// Project 0 — tracking / abstract clock
// Namespace: p0::tracking
namespace p0::tracking {

// Abstract clock interface.
// Decouples time sources (simulation, real, replay) from the tracking layer.
class IClock {
 public:
  virtual ~IClock() = default;

  // Returns current time in seconds.
  virtual double now() const = 0;
};

// Simulation clock driven by a fixed or variable timestep.
class SimulationClock : public IClock {
 public:
  explicit SimulationClock(double start_time = 0.0);

  void set_time(double t);
  void advance(double dt);

  double now() const override;

 private:
  double time_ = 0.0;
};

// Real-world clock using std::chrono steady clock.
class RealClock : public IClock {
 public:
  double now() const override;
};

// Replay clock that advances at a configurable rate relative to real time.
class ReplayClock : public IClock {
 public:
  explicit ReplayClock(double start_time = 0.0, double rate = 1.0);

  void set_rate(double rate);
  void set_time(double t);
  void update();

  double now() const override;

 private:
  double time_ = 0.0;
  double rate_ = 1.0;
  double last_real_time_ = 0.0;
  bool initialized_ = false;
};

}  // namespace p0::tracking
