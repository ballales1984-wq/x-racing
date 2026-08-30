#pragma once

#include "common.h"
#include "tracking/position_sample.h"

// Project 0 — tracking / GPS provider interfaces
// Namespace: p0::tracking
namespace p0::tracking {

// Abstract position provider.
// Implementations produce PositionSample data without exposing their source.
class IPositionProvider {
 public:
  virtual ~IPositionProvider() = default;

  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual bool is_running() const = 0;

  // Polls for a new sample. Returns true and fills `sample` only when a
  // fresh sample is available; returns false if the source has produced
  // nothing new since the last poll (e.g. a 10 Hz GPS polled at 120 Hz).
  virtual bool poll(PositionSample& sample) = 0;

  virtual double update_rate_hz() const = 0;
};

// Trajectory source for ReplayGPS.
// Provides deterministic samples for a given absolute time.
class ITrajectorySource {
 public:
  virtual ~ITrajectorySource() = default;

  virtual PositionSample sample_at(double time) = 0;
  virtual double duration() const = 0;
};

// Abstract GPS device interface for RealGPS.
// Decouples transport (serial, USB, Bluetooth, network) from the provider.
class IGPSDevice {
 public:
  virtual ~IGPSDevice() = default;

  virtual bool connect() = 0;
  virtual void disconnect() = 0;

  // Reads a single sample from the device into the output parameter.
  // Returns true on success.
  virtual bool read(PositionSample& sample) = 0;
};

}  // namespace p0::tracking
