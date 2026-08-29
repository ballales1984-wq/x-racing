#include "debug/debug_network.h"
#include <algorithm>
#include <cstdio>

namespace p0::debug {

void DebugNetwork::initialize() {
  reset();
}

void DebugNetwork::shutdown() {
  reset();
}

void DebugNetwork::reset() {
  diagnostics_ = NetworkDiagnostics{};
  sent_packets_prev_ = 0;
  recv_packets_prev_ = 0;
  total_sent_bytes_ = 0.0;
  total_recv_bytes_ = 0.0;
  ping_samples_ = 0;
  ping_sum_ = 0.0;
  last_snapshot_time_ = 0.0;
}

void DebugNetwork::analyze_snapshot(const p0::network::WorldSnapshot& snapshot) {
  diagnostics_.snapshot_count++;
  last_snapshot_time_ = 0.0;

  diagnostics_.player_count = snapshot.car_count;

  for (int i = 0; i < snapshot.car_count; ++i) {
    const auto& car = snapshot.cars[i];
    total_sent_bytes_ += sizeof(p0::network::CarSnapshot);
    (void)car;
  }

  diagnostics_.packets_received = diagnostics_.snapshot_count;
  diagnostics_.packets_sent = diagnostics_.snapshot_count;
  diagnostics_.total_bytes_sent = total_sent_bytes_;
  diagnostics_.total_bytes_received = total_recv_bytes_;

  const int total = diagnostics_.packets_sent + diagnostics_.packets_received;
  if (total > 0) {
    const int expected = std::max(diagnostics_.packets_sent, diagnostics_.packets_received);
    const int lost = std::abs(diagnostics_.packets_sent - diagnostics_.packets_received);
    diagnostics_.packet_loss_percent = total > 0 ? (100.0 * lost / expected) : 0.0;
  }
}

void DebugNetwork::update_stats() {
  if (!diagnostics_.connected) return;
  last_snapshot_time_ += 1.0 / 60.0;
  diagnostics_.last_snapshot_age_s = last_snapshot_time_;

  if (last_snapshot_time_ > 2.0) {
    diagnostics_.recent_events.push_back("Snapshot timeout: " +
        std::to_string(static_cast<int>(last_snapshot_time_)) + "s");
    if (diagnostics_.recent_events.size() > 32) {
      diagnostics_.recent_events.erase(diagnostics_.recent_events.begin());
    }
  }
}

}
