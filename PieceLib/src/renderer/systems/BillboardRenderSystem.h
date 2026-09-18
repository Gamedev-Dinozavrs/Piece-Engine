#pragma once

#include <renderer/FrameInfo.h>
#include <renderer/RendererContext.h>
#include <glm/glm.hpp>

namespace Piece {

struct BillboardPushConstants {
    glm::mat4 viewProj{1.0f};
    glm::vec4 worldPositionSize{0.0f, 0.0f, 0.0f, 0.4f}; // xyz = center, w = world-space size
    glm::vec4 cameraRight{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec4 cameraUp{0.0f, 1.0f, 0.0f, 0.0f};
    glm::vec4 tint{1.0f};
    glm::ivec4 entityData{0}; // x/y = UUID halves
};

namespace BillboardRenderSystem {

void Record(const RendererContext& ctx, const FrameInfo& frameInfo);

} // namespace BillboardRenderSystem

} // namespace Piece
