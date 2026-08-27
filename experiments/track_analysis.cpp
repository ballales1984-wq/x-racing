// Project 0 — track geometry analysis experiment
// Prints track length, position, and curvature at regular intervals.
// Useful for verifying track construction and identifying high-curvature zones.
#include "track/track.h"
#include <iostream>
#include <iomanip>

using namespace p0::track;

int main() {
  // Build the default track and print its total length.
  Track track;
  std::cout << "Track length: " << track.length() << " m\n";

  // Sample curvature every 500 meters and print position + curvature.
  std::cout << std::fixed << std::setprecision(2);
  for (double d = 0.0; d < track.length(); d += 500.0) {
    const auto& tp = track.at(d);
    std::cout << "d=" << d << " pos=(" << tp.position.x() << ", " << tp.position.y()
              << ") curv=" << tp.curvature << "\n";
  }

  return 0;
}
