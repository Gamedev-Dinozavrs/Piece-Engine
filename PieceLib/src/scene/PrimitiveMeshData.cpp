#include <PiecePCH.h>

#include <scene/PrimitiveMeshData.h>

namespace Piece {

namespace PrimitiveMeshDataFactory {

PrimitiveMeshData CreateQuad() {
    PrimitiveMeshData data{};
    data.vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.2f, 0.2f}, {0, 0}},
        {{0.5f, -0.5f, 0.0f}, {0.2f, 1.0f, 0.2f}, {1, 0}},
        {{0.5f, 0.5f, 0.0f}, {0.2f, 0.2f, 1.0f}, {1, 1}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.2f}, {0, 1}},
    };

    data.indices = {0, 1, 2, 2, 3, 0};
    return data;
}

PrimitiveMeshData CreateCube() {
    PrimitiveMeshData data{};
    data.vertices = {
        // Front face (+Z)
        {{-0.5f, -0.5f, 0.5f}, {1, 0, 0}, {0, 0}},
        {{0.5f, -0.5f, 0.5f}, {0, 1, 0}, {1, 0}},
        {{0.5f, 0.5f, 0.5f}, {0, 0, 1}, {1, 1}},
        {{-0.5f, 0.5f, 0.5f}, {1, 1, 0}, {0, 1}},

        // Back face (-Z)
        {{-0.5f, -0.5f, -0.5f}, {1, 0, 1}, {0, 0}},
        {{0.5f, -0.5f, -0.5f}, {0, 1, 1}, {1, 0}},
        {{0.5f, 0.5f, -0.5f}, {0.5, 0.5, 0.5}, {1, 1}},
        {{-0.5f, 0.5f, -0.5f}, {0.8, 0.2, 0.2}, {0, 1}},
    };

    data.indices = {
        0, 1, 2, 2, 3, 0,
        4, 6, 5, 4, 7, 6,
        3, 2, 6, 6, 7, 3,
        4, 5, 1, 1, 0, 4,
        1, 5, 6, 6, 2, 1,
        4, 0, 3, 3, 7, 4,
    };

    return data;
}

} // namespace PrimitiveMeshDataFactory

} // namespace Piece
