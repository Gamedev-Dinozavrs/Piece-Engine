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

    constexpr uint32_t stacks = 32;
    constexpr uint32_t slices = 48;
    constexpr float radius = 0.5f;

    const auto addVertex = [&data](const glm::vec3& position, const glm::vec2& uv) {
        data.vertices.push_back({
            position,
            {uv.x, uv.y, 1.0f - uv.x},
            uv,
            glm::normalize(position)
        });
    };

    addVertex({0.0f, radius, 0.0f}, {0.5f, 0.0f});

    for (uint32_t stack = 1; stack < stacks; ++stack) {
        const float v = static_cast<float>(stack) / static_cast<float>(stacks);
        const float phi = v * glm::pi<float>();
        for (uint32_t slice = 0; slice <= slices; ++slice) {
            const float u = static_cast<float>(slice) / static_cast<float>(slices);
            const float theta = u * glm::two_pi<float>();
            const glm::vec3 position{
                radius * std::sin(phi) * std::cos(theta),
                radius * std::cos(phi),
                radius * std::sin(phi) * std::sin(theta)};
            addVertex(position, {u, v});
        }
    }

    const uint32_t bottomIndex = static_cast<uint32_t>(data.vertices.size());
    addVertex({0.0f, -radius, 0.0f}, {0.5f, 1.0f});

    for (uint32_t slice = 0; slice < slices; ++slice) {
        const uint32_t firstRing = 1 + slice;
        data.indices.insert(data.indices.end(), {0, firstRing + 1, firstRing});
    }

    const uint32_t ringCount = stacks - 1;
    for (uint32_t ring = 0; ring + 1 < ringCount; ++ring) {
        const uint32_t first = 1 + ring * (slices + 1);
        const uint32_t second = first + slices + 1;
        for (uint32_t slice = 0; slice < slices; ++slice) {
            data.indices.insert(data.indices.end(), {
                first + slice,
                second + slice + 1,
                second + slice,
                first + slice,
                first + slice + 1,
                second + slice + 1});
        }
    }

    const uint32_t lastRing = 1 + (ringCount - 1) * (slices + 1);
    for (uint32_t slice = 0; slice < slices; ++slice) {
        data.indices.insert(data.indices.end(), {
            lastRing + slice + 1,
            lastRing + slice,
            bottomIndex});
    }

    return data;
}

} // namespace PrimitiveMeshDataFactory

} // namespace Piece

