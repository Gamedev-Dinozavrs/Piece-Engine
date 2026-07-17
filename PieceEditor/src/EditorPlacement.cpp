#include "EditorPlacement.h"

#include <renderer/Renderer.h>

namespace Piece {

namespace EditorPlacement {

void SpawnQuadInView() {
    Renderer::CreateQuadInView();
}

void SpawnCubeInView() {
    Renderer::CreateCubeInView();
}

bool SpawnPointLightInView() {
    return Renderer::CreatePointLightInView();
}

} // namespace EditorPlacement

} // namespace Piece
