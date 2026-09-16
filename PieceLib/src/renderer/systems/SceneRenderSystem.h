#pragma once

#include <renderer/FrameInfo.h>
#include <renderer/RendererContext.h>
#include <glm/glm.hpp>

namespace Piece {

struct ScenePushConstants {
    glm::mat4 mvp{1.0f};
    glm::mat4 model{1.0f};
    glm::vec4 materialFactors{1.0f, 1.0f, 0.0f, 0.0f}; // x = roughnessFactor, y = metallicFactor
    glm::vec4 baseColor{1.0f};
    glm::vec4 emissiveColor{0.0f}; // rgb = color, a = enabled
    glm::vec4 bloomParams{0.8f, 0.35f, 2.0f, 1.0f}; // x = threshold, y = intensity, z = radius, w = enabled
    glm::vec4 bloomFlags{0.0f}; // x = emissive bloom enabled
    glm::ivec4 materialData{0}; // x = normalSource, y = materialFlags, z/w = UUID halves
};

namespace SceneRenderSystem {

void Record(const RendererContext& ctx, const FrameInfo& frameInfo);

} // namespace SceneRenderSystem

} // namespace Piece