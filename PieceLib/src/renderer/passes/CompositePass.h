#pragma once

#include <renderer/FrameInfo.h>
#include <renderer/RendererContext.h>

namespace Piece {

namespace CompositePass {

void Record(const RendererContext& ctx, const FrameInfo& frameInfo);

} // namespace CompositePass

} // namespace Piece
