#include <PiecePCH.h>

#include <scene/PrimitiveMeshData.h>

#include <glm/gtc/constants.hpp>
#include <array>
#include <cmath>

namespace Piece {

namespace PrimitiveMeshDataFactory {

PrimitiveMeshData CreateQuad() {
    PrimitiveMeshData data{};
    data.vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.2f, 0.2f}, {0, 0}, {0.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.2f, 1.0f, 0.2f}, {1, 0}, {0.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.2f, 0.2f, 1.0f}, {1, 1}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.2f}, {0, 1}, {0.0f, 0.0f, 1.0f}},
    };

    data.indices = {0, 1, 2, 2, 3, 0};
    return data;
}

PrimitiveMeshData CreateCube() {
    PrimitiveMeshData data{};
    const auto addFace = [&data](const glm::vec3& normal, const std::array<glm::vec3, 4>& positions) {
        const uint32_t first = static_cast<uint32_t>(data.vertices.size());
        const std::array<glm::vec2, 4> uvs = {
            glm::vec2{0.0f, 0.0f}, glm::vec2{1.0f, 0.0f},
            glm::vec2{1.0f, 1.0f}, glm::vec2{0.0f, 1.0f}};
        const std::array<glm::vec3, 4> colors = {
            glm::vec3{1.0f, 0.2f, 0.2f}, glm::vec3{0.2f, 1.0f, 0.2f},
            glm::vec3{0.2f, 0.2f, 1.0f}, glm::vec3{1.0f, 1.0f, 0.2f}};

        for (size_t i = 0; i < positions.size(); ++i) {
            data.vertices.push_back({positions[i], colors[i], uvs[i], normal});
        }

        data.indices.insert(data.indices.end(), {
            first, first + 1, first + 2,
            first + 2, first + 3, first});
    };

    addFace({0.0f, 0.0f, 1.0f}, {
        glm::vec3{-0.5f, -0.5f, 0.5f}, glm::vec3{0.5f, -0.5f, 0.5f},
        glm::vec3{0.5f, 0.5f, 0.5f}, glm::vec3{-0.5f, 0.5f, 0.5f}});
    addFace({0.0f, 0.0f, -1.0f}, {
        glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec3{0.5f, -0.5f, -0.5f},
        glm::vec3{0.5f, 0.5f, -0.5f}, glm::vec3{-0.5f, 0.5f, -0.5f}});
    addFace({0.0f, 1.0f, 0.0f}, {
        glm::vec3{-0.5f, 0.5f, 0.5f}, glm::vec3{0.5f, 0.5f, 0.5f},
        glm::vec3{0.5f, 0.5f, -0.5f}, glm::vec3{-0.5f, 0.5f, -0.5f}});
    addFace({0.0f, -1.0f, 0.0f}, {
        glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec3{0.5f, -0.5f, -0.5f},
        glm::vec3{0.5f, -0.5f, 0.5f}, glm::vec3{-0.5f, -0.5f, 0.5f}});
    addFace({1.0f, 0.0f, 0.0f}, {
        glm::vec3{0.5f, -0.5f, 0.5f}, glm::vec3{0.5f, -0.5f, -0.5f},
        glm::vec3{0.5f, 0.5f, -0.5f}, glm::vec3{0.5f, 0.5f, 0.5f}});
    addFace({-1.0f, 0.0f, 0.0f}, {
        glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec3{-0.5f, -0.5f, 0.5f},
        glm::vec3{-0.5f, 0.5f, 0.5f}, glm::vec3{-0.5f, 0.5f, -0.5f}});

    return data;
}

PrimitiveMeshData CreateSphere() {
    PrimitiveMeshData data{};

    constexpr uint32_t stacks = 16;
    constexpr uint32_t slices = 24;
    constexpr float radius = 0.5f;

    for (uint32_t stack = 0; stack <= stacks; ++stack) {
        const float v = static_cast<float>(stack) / static_cast<float>(stacks);
        const float phi = v * glm::pi<float>();

        for (uint32_t slice = 0; slice <= slices; ++slice) {
            const float u = static_cast<float>(slice) / static_cast<float>(slices);
            const float theta = u * glm::two_pi<float>();

            const float x = radius * std::sin(phi) * std::cos(theta);
            const float y = radius * std::cos(phi);
            const float z = radius * std::sin(phi) * std::sin(theta);

            data.vertices.push_back({
                {x, y, z},
                {u, v, 1.0f - u},
                {u, v},
                glm::normalize(glm::vec3{x, y, z})
            });
        }
    }

    for (uint32_t stack = 0; stack < stacks; ++stack) {
        for (uint32_t slice = 0; slice < slices; ++slice) {
            const uint32_t first = stack * (slices + 1) + slice;
            const uint32_t second = first + slices + 1;

            data.indices.push_back(first);
            data.indices.push_back(second);
            data.indices.push_back(first + 1);

            data.indices.push_back(second);
            data.indices.push_back(second + 1);
            data.indices.push_back(first + 1);
        }
    }

    return data;
}

} // namespace PrimitiveMeshDataFactory

} // namespace Piece
