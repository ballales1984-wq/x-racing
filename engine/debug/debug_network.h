#pragma once

#include "common.h"
#include "network/network_protocol.h"
#include "network/snapshot.h"
#include <string>
#include <vector>

namespace p0::debug {

struct NetworkDiagnostics {
  bool connected = false;
  bool is_host = false;
  int player_count = 0;
  int avg_ping_ms = 0;
  int min_ping_ms = 9999;
  int max_ping_ms = 0;
  int packets_sent = 0;
  int packets_received = 0;
  double packet_loss_percent = 0.0;
  p0::network::ConnectionState connection_state = p0::network::ConnectionState::DISCONNECTED;
  double last_snapshot_age_s = 0.0;
  int snapshot_count = 0;
  double total_bytes_sent = 0.0;
  double total_bytes_received = 0.0;
  bool desync_detected = false;
  double desync_distance_m = 0.0;
  std::vector<std::string> recent_events;
};

class DebugNetwork {
 public:
  DebugNetwork() = default;

  void initialize();
  void shutdown();

  void analyze_snapshot(const p0::network::WorldSnapshot& snapshot);
  void update_stats();
  void reset();

  const NetworkDiagnostics& current_diagnostics() const { return diagnostics_; }
  void set_connected(bool c) { diagnostics_.connected = c; }
  void set_is_host(bool h) { diagnostics_.is_host = h; }

 private:
  NetworkDiagnostics diagnostics_;
  int sent_packets_prev_ = 0;
  int recv_packets_prev_ = 0;
  double total_sent_bytes_ = 0.0;
  double total_recv_bytes_ = 0.0;
  int ping_samples_ = 0;
  double ping_sum_ = 0.0;
  double last_snapshot_time_ = 0.0;
};

}
