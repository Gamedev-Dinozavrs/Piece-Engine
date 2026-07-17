#pragma once

#include <scene/Mesh.h>

#include <cstdint>
#include <vector>

namespace Piece {

struct PrimitiveMeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

namespace PrimitiveMeshDataFactory {

PrimitiveMeshData CreateQuad();
PrimitiveMeshData CreateCube();
PrimitiveMeshData CreateSphere();

} // namespace PrimitiveMeshDataFactory

} // namespace Piece
