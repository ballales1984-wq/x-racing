#include "track/track.h"
#include <iostream>
#include <iomanip>

// Project 0 — track geometry analysis experiment
// Prints track length, position, and curvature at regular intervals.
using namespace p0::track;

int main() {
  Track track;
  std::cout << "Track length: " << track.length() << " m\n";

  std::cout << std::fixed << std::setprecision(2);
  for (double d = 0.0; d < track.length(); d += 500.0) {
    const auto& tp = track.at(d);
    std::cout << "d=" << d << " pos=(" << tp.position.x() << ", " << tp.position.y()
              << ") curv=" << tp.curvature << "\n";
  }

  return 0;
}
