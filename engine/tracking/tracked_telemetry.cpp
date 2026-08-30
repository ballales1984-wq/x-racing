#include "tracking/tracked_telemetry.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace p0::tracking {

TrackedTelemetry::TrackedTelemetry(const std::string& output_path)
    : output_path_(output_path) {}

void TrackedTelemetry::record(const TrackPosition& pos,
                               const PositionSample& gps,
                               double grip,
                               double surface_temp,
                               double rubber,
                               const LapSystem& lap_system) {
  TrackedFrame frame{};
  frame.timestamp = gps.timestamp;
  frame.lap_number = lap_system.completed_laps();
  frame.s = pos.s;
  frame.lateral = pos.lateral;
  frame.speed = gps.speed;
  frame.heading = pos.heading;
  frame.latitude = gps.latitude;
  frame.longitude = gps.longitude;
  frame.altitude = gps.altitude;
  frame.gps_speed = gps.speed;
  frame.gps_heading = gps.heading;
  frame.gps_accuracy = gps.horizontal_accuracy;
  frame.grip = grip;
  frame.surface_temp = surface_temp;
  frame.rubber = rubber;
  frame.lap_time = lap_system.current_lap_time();
  frames_.push_back(frame);
}

void TrackedTelemetry::save_csv() const {
  std::ofstream file(output_path_);
  if (!file.is_open()) return;

  file << "timestamp,lap_number,s,lateral,speed,heading,"
          "latitude,longitude,altitude,gps_speed,gps_heading,gps_accuracy,"
          "grip,surface_temp,rubber,lap_time\n";

  file << std::fixed << std::setprecision(6);
  for (const auto& frame : frames_) {
    file << frame.timestamp << ","
         << frame.lap_number << ","
         << frame.s << ","
         << frame.lateral << ","
         << frame.speed << ","
         << frame.heading << ","
         << frame.latitude << ","
         << frame.longitude << ","
         << frame.altitude << ","
         << frame.gps_speed << ","
         << frame.gps_heading << ","
         << frame.gps_accuracy << ","
         << frame.grip << ","
         << frame.surface_temp << ","
         << frame.rubber << ","
         << frame.lap_time << "\n";
  }
}

void TrackedTelemetry::clear() {
  frames_.clear();
  current_lap_ = 0;
}

}  // namespace p0::tracking
