#pragma once

#include <renderer/FrameInfo.h>
#include <renderer/RendererContext.h>

namespace Piece {

namespace GeometryPass {

void Record(const RendererContext& ctx, const FrameInfo& frameInfo);

} // namespace GeometryPass

} // namespace Piece
