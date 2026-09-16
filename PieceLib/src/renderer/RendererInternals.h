#pragma once

#include <renderer/RendererContext.h>

namespace Piece {

namespace RendererInternals {

void DestroyOffscreenResources(RendererContext& ctx);
void DestroyCompositeResources(RendererContext& ctx);
void DestroyGeometryRenderPass(RendererContext& ctx);
void DestroyLightingRenderPass(RendererContext& ctx);
void DestroyBloomRenderPass(RendererContext& ctx);
void DestroyPipelineLayouts(RendererContext& ctx);

void CreateGeometryRenderPass(RendererContext& ctx);
void CreateLightingRenderPass(RendererContext& ctx);
void CreateBloomRenderPass(RendererContext& ctx);
void CreateOffscreenResources(RendererContext& ctx);
void CreateCompositeResources(RendererContext& ctx);
void CreateGraphicsPipeline(RendererContext& ctx);

} // namespace RendererInternals

} // namespace Piece
