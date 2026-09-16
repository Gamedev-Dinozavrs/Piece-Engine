#include <PiecePCH.h>

#include <renderer/passes/LightingPass.h>

#include <renderer/passes/CompositePass.h>
#include <renderer/SwapChain.h>
#include <renderer/systems/LightingRenderSystem.h>
#include <renderer/systems/UIRenderSystem.h>
#include <scene/World.h>

#include <array>

namespace Piece {

namespace LightingPass {

void Record(RendererContext& ctx, const FrameInfo& frameInfo) {
	VkClearValue lightingClearValues[1] = {};
	lightingClearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};

	VkRenderPassBeginInfo lightingPassInfo{};
	lightingPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	lightingPassInfo.renderPass = ctx.lightingRenderPass;
	lightingPassInfo.framebuffer = ctx.offscreenFrames[frameInfo.imageIndex].lightingFramebuffer;
	lightingPassInfo.renderArea.offset = {0, 0};
	lightingPassInfo.renderArea.extent = frameInfo.swapChainExtent;
	lightingPassInfo.clearValueCount = 1;
	lightingPassInfo.pClearValues = lightingClearValues;

	vkCmdBeginRenderPass(frameInfo.commandBuffer, &lightingPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	CompositePass::Record(ctx, frameInfo);
	vkCmdEndRenderPass(frameInfo.commandBuffer);
	LightingRenderSystem::RecordBloom(ctx, frameInfo);

	Pipeline& finalPipeline = *ctx.presentPipeline;
	VkDescriptorSet finalDescriptorSet = ctx.presentDescriptorSets[frameInfo.imageIndex];

	VkClearValue presentClearValues[2] = {};
	presentClearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
	presentClearValues[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo presentPassInfo{};
	presentPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	presentPassInfo.renderPass = ctx.presentRenderPass;
	presentPassInfo.framebuffer = ctx.swapChainWrapper->getFrameBuffer(static_cast<int>(frameInfo.imageIndex));
	presentPassInfo.renderArea.offset = {0, 0};
	presentPassInfo.renderArea.extent = frameInfo.swapChainExtent;
	presentPassInfo.clearValueCount = 2;
	presentPassInfo.pClearValues = presentClearValues;

	vkCmdBeginRenderPass(frameInfo.commandBuffer, &presentPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	LightingRenderSystem::RecordPresent(
		frameInfo.commandBuffer,
		frameInfo.swapChainExtent,
		finalPipeline,
		ctx.presentPipelineLayout,
		finalDescriptorSet,
		frameInfo.globalDescriptorSet);
	UiRenderSystem::Record(frameInfo.commandBuffer);
	vkCmdEndRenderPass(frameInfo.commandBuffer);
}

} // namespace LightingPass

} // namespace Piece
