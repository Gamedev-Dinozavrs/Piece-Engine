#include <PiecePCH.h>

#include <renderer/passes/CompositePass.h>

#include <renderer/systems/LightingRenderSystem.h>
#include <renderer/systems/UIRenderSystem.h>

namespace Piece {

namespace CompositePass {

void Record(const RendererContext& ctx, const FrameInfo& frameInfo) {
	LightingRenderSystem::RecordComposite(
		frameInfo.commandBuffer,
		frameInfo.swapChainExtent,
		*ctx.lightingPipeline,
		ctx.lightingPipelineLayout,
		ctx.compositeDescriptorSets[frameInfo.imageIndex],
		ctx.globalDescriptorSets[ctx.currentFrame]);
	UiRenderSystem::Record(frameInfo.commandBuffer);
}

} // namespace CompositePass

} // namespace Piece
