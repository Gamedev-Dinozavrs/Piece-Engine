#include <PiecePCH.h>

#include <scene/World.h>

#include <renderer/Renderer.h>

namespace Piece {

namespace World {

uint32_t SpawnQuad(const SpawnTransform& transform) {
    return Renderer::CreateQuad(transform.position, transform.rotation, transform.scale);
}

uint32_t SpawnCube(const SpawnTransform& transform) {
    return Renderer::CreateCube(transform.position, transform.rotation, transform.scale);
}

} // namespace World

} // namespace Piece
