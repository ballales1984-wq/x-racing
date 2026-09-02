#pragma once

#include <array>
#include <cstdint>
#include "rendering/scene.h"

namespace xe {

std::array<Vertex, 24> MakeCubeVertices();
std::array<uint16_t, 36> MakeCubeIndices();

}  // namespace xe