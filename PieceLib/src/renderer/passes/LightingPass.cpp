#include <PiecePCH.h>

#include <renderer/passes/LightingPass.h>

#include <renderer/passes/CompositePass.h>
#include <renderer/SwapChain.h>

namespace Piece {

namespace LightingPass {

void Record(const RendererContext& ctx, const FrameInfo& frameInfo) {
	VkClearValue lightingClearValues[2] = {};
	lightingClearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
	lightingClearValues[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo lightingPassInfo{};
	lightingPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	lightingPassInfo.renderPass = ctx.lightingRenderPass;
	lightingPassInfo.framebuffer = ctx.swapChainWrapper->getFrameBuffer(static_cast<int>(frameInfo.imageIndex));
	lightingPassInfo.renderArea.offset = {0, 0};
	lightingPassInfo.renderArea.extent = frameInfo.swapChainExtent;
	lightingPassInfo.clearValueCount = 2;
	lightingPassInfo.pClearValues = lightingClearValues;

	vkCmdBeginRenderPass(frameInfo.commandBuffer, &lightingPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	CompositePass::Record(ctx, frameInfo);
	vkCmdEndRenderPass(frameInfo.commandBuffer);
}

} // namespace LightingPass

} // namespace Piece
