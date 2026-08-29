// X-Racing Debug Agent Demo
// Standalone executable that runs a headless simulation with full debug instrumentation.
// Demonstrates physics diagnostics, AI analysis, network monitoring, and performance profiling.
#include "engine/simulation/simulation.h"
#include "engine/telemetry/telemetry.h"
#include "engine/ai/ai_driver.h"
#include "engine/input/platform/auto_input.h"
#include "debug/debug_agent.h"
#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>

namespace p0 {

static void run_debug_demo() {
  std::cout << "=== X-RACING DEBUG AGENT ===\n\n";

  p0::track::Track track;
  p0::simulation::Simulation sim;
  p0::telemetry::Telemetry tel;
  p0::debug::DebugAgent debug_agent;

  p0::debug::DebugConfig cfg;
  cfg.enabled = true;
  cfg.console_enabled = true;
  cfg.physics_warnings = true;
  cfg.detect_off_track = true;
  cfg.detect_spin = true;
  cfg.auto_telemetry = true;
  cfg.telemetry_interval_s = 2.0;
  cfg.export_directory = "./debug_output";
  debug_agent.initialize(cfg);

  sim.set_track(track);
  debug_agent.set_simulation(&sim);
  debug_agent.set_telemetry(&tel);
  debug_agent.set_track(&track);

  p0::vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 0.0;
  sim.reset(initial);
  tel.clear();

  ai::AIDriverParams ai_params;
  ai_params.difficulty = ai::AIDifficulty::MEDIUM;
  ai_params.look_ahead_distance = 40.0;
  ai::AIDriver ai_driver(ai_params);
  ai_driver.set_track(track);

  input::AutoInputManager auto_input;
  auto_input.set_mode(input::AutoInputMode::AI_ASSIST);

  const double target_dt = 1.0 / 60.0;
  const int total_frames = 1800;
  const double sim_speed = 1.0;

  for (int frame = 0; frame < total_frames; ++frame) {
    auto frame_start = std::chrono::high_resolution_clock::now();

    ai_driver.update(sim.state(), target_dt);
    input::InputState ai_input = ai_driver.poll();

    auto_input.update(sim.state(), target_dt);
    input::InputState final_input = auto_input.blend(ai_input, target_dt);

    sim::SimulationResult result = sim.step(final_input);

    debug_agent.record_ai_input(0, final_input, ai_params);
    debug_agent.record_frame(result.state, target_dt);
    debug_agent.update(target_dt, target_dt);

    tel.record(result.state, target_dt);

    if (frame % 120 == 0) {
      const auto& snap = debug_agent.last_snapshot();
      std::cout << "[F:" << std::setw(4) << frame << "] "
                << "Speed: " << std::setw(5) << std::fixed << std::setprecision(0)
                << (snap.state.speed * 3.6) << " km/h | "
                << "Lap: " << snap.state.lap << " | "
                << "Gear: " << snap.state.gear << " | "
                << "RPM: " << std::setw(4) << std::setprecision(0) << snap.state.rpm << " | "
                << "Phys: " << std::setw(5) << std::setprecision(2)
                << snap.profiler.last_physics_time_ms << "ms | "
                << "FPS: " << std::setw(5) << std::setprecision(1)
                << snap.profiler.fps << "\n";
    }

    if (result.collision) {
      std::cout << "\n*** COLLISION at frame " << frame << " ***\n";
      sim.respawn();
    }

    auto frame_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = frame_end - frame_start;
    double sleep_time = (target_dt / sim_speed) - elapsed.count();
    if (sleep_time > 0.0) {
      std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
    }
  }

  std::cout << "\n=== DEBUG AGENT SUMMARY ===\n";
  const auto& final_snap = debug_agent.last_snapshot();
  std::cout << "Total frames:    " << final_snap.frame_count << "\n";
  std::cout << "Sim time:        " << std::fixed << std::setprecision(2)
            << final_snap.sim_time << "s\n";
  std::cout << "Avg FPS:         " << std::setprecision(1)
            << final_snap.profiler.fps << "\n";
  std::cout << "Avg frame time:  " << std::setprecision(2)
            << final_snap.profiler.avg_frame_time_ms << "ms\n";
  std::cout << "Physics ratio:   " << std::setprecision(1)
            << (final_snap.profiler.physics_ratio * 100.0) << "%\n";
  std::cout << "Log entries:     " << debug_agent.logs().size() << "\n";
  std::cout << "Snapshots:       " << final_snap.frame_count << "\n";

  std::cout << "\n=== PHYSICS STATUS ===\n";
  std::cout << "Tire status:     " << final_snap.physics.tire_status << "\n";
  std::cout << "Suspension:      " << final_snap.physics.suspension_status << "\n";
  std::cout << "Aero status:     " << final_snap.physics.aero_status << "\n";
  std::cout << "Peak slip angle: " << std::setprecision(4)
            << final_snap.physics.peak_slip_angle << " rad\n";
  std::cout << "Peak slip ratio: " << std::setprecision(4)
            << final_snap.physics.peak_slip_ratio << "\n";

  std::cout << "\n=== CONSOLE COMMANDS ===\n";
  std::cout << "Type 'help' for available commands\n";
  std::cout << "Examples: status, physics, ai, network, profile, track\n";
  std::cout << "          telemetry, export, logs, inspect speed\n\n";

  std::string line;
  while (true) {
    std::cout << "debug> ";
    if (!std::getline(std::cin, line)) break;
    if (line == "quit" || line == "exit") break;
    if (line.empty()) continue;
    debug_agent.process_console_command(line);
  }

  debug_agent.export_telemetry_csv("./debug_output/final_telemetry.csv");
  debug_agent.export_snapshot_json("./debug_output/final_snapshot.json");
  debug_agent.export_logs_csv("./debug_output/debug_log.csv");
  std::cout << "\nDebug data exported to ./debug_output/\n";

  debug_agent.shutdown();
}

}

int main() {
  try {
    p0::run_debug_demo();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << "\n";
    return 1;
  }
}
