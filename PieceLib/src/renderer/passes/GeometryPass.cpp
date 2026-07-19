#include <PiecePCH.h>

#include <renderer/passes/GeometryPass.h>

#include <renderer/systems/SceneRenderSystem.h>

namespace Piece {

namespace GeometryPass {

void Record(const RendererContext& ctx, const FrameInfo& frameInfo) {
	VkClearValue geometryClearValues[5] = {};
	geometryClearValues[0].color = {{0.0f, 0.0f, 0.0f, -1.0f}};
	geometryClearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
	geometryClearValues[2].color = {{0.5f, 0.5f, 1.0f, 0.0f}};
	geometryClearValues[3].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
	geometryClearValues[4].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo geometryPassInfo{};
	geometryPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	geometryPassInfo.renderPass = ctx.geometryRenderPass;
	geometryPassInfo.framebuffer = ctx.offscreenFrames[frameInfo.imageIndex].framebuffer;
	geometryPassInfo.renderArea.offset = {0, 0};
	geometryPassInfo.renderArea.extent = frameInfo.swapChainExtent;
	geometryPassInfo.clearValueCount = 5;
	geometryPassInfo.pClearValues = geometryClearValues;

	vkCmdBeginRenderPass(frameInfo.commandBuffer, &geometryPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	FrameInfo geometryFrameInfo = frameInfo;
	geometryFrameInfo.renderPass = ctx.geometryRenderPass;
	geometryFrameInfo.framebuffer = ctx.offscreenFrames[frameInfo.imageIndex].framebuffer;
	geometryFrameInfo.pipelineLayout = ctx.geometryPipelineLayout;
	geometryFrameInfo.pipeline = ctx.geometryPipeline.get();
	SceneRenderSystem::Record(ctx, geometryFrameInfo);

	vkCmdEndRenderPass(frameInfo.commandBuffer);
}

} // namespace GeometryPass

} // namespace Piece
