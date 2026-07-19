#pragma once

#include <renderer/FrameInfo.h>
#include <renderer/RendererContext.h>

namespace Piece {

namespace LightingPass {

void Record(RendererContext& ctx, const FrameInfo& frameInfo);

} // namespace LightingPass

} // namespace Piece
