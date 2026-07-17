#include "EditorPlacement.h"

#include <renderer/Renderer.h>

namespace Piece {

namespace EditorPlacement {

void SpawnPrimitiveInView(PrimitiveType primitiveType) {
    switch (primitiveType) {
    case PrimitiveType::Quad:
        Renderer::CreateQuadInView();
        break;
    case PrimitiveType::Cube:
        Renderer::CreateCubeInView();
        break;
    case PrimitiveType::Sphere:
        Renderer::CreateSphereInView();
        break;
    default:
        break;
    }
}

void SpawnQuadInView() {
    SpawnPrimitiveInView(PrimitiveType::Quad);
}

void SpawnCubeInView() {
    SpawnPrimitiveInView(PrimitiveType::Cube);
}

void SpawnSphereInView() {
    SpawnPrimitiveInView(PrimitiveType::Sphere);
}

bool SpawnPointLightInView() {
    return Renderer::CreatePointLightInView();
}

} // namespace EditorPlacement

} // namespace Piece
