#pragma once

#include <scene/RenderObject.h>

namespace Piece {

namespace EditorPlacement {

bool DrawCreateMenu();
void SpawnPrimitiveInView(PrimitiveType primitiveType);
void SpawnQuadInView();
void SpawnCubeInView();
void SpawnSphereInView();
bool SpawnPointLightInView();

} // namespace EditorPlacement

} // namespace Piece
