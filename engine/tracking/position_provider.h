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

  // Retrieves the latest sample. Returns false if no new data is available.
  virtual bool update(PositionSample& sample) = 0;

  virtual double update_rate_hz() const = 0;
};

// Trajectory source for SimulatedGPS.
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
