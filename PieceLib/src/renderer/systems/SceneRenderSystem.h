#pragma once

#include <renderer/FrameInfo.h>
#include <renderer/RendererContext.h>
#include <glm/glm.hpp>

namespace Piece {

struct ScenePushConstants {
    glm::mat4 mvp{1.0f};
    glm::mat4 model{1.0f};
};

namespace SceneRenderSystem {

void Record(const RendererContext& ctx, const FrameInfo& frameInfo);

} // namespace SceneRenderSystem

} // namespace Piece