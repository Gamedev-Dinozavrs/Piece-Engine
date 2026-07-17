#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace Piece {

struct SpawnTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

namespace World {

uint32_t SpawnQuad(const SpawnTransform& transform);
uint32_t SpawnCube(const SpawnTransform& transform);

} // namespace World

} // namespace Piece
