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

	VkImageMemoryBarrier lightingToFxaBarrier{};
	lightingToFxaBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	lightingToFxaBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	lightingToFxaBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	lightingToFxaBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	lightingToFxaBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	lightingToFxaBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	lightingToFxaBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	lightingToFxaBarrier.image = ctx.offscreenFrames[frameInfo.imageIndex].lightingColorImage;
	lightingToFxaBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	lightingToFxaBarrier.subresourceRange.baseMipLevel = 0;
	lightingToFxaBarrier.subresourceRange.levelCount = 1;
	lightingToFxaBarrier.subresourceRange.baseArrayLayer = 0;
	lightingToFxaBarrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(
		frameInfo.commandBuffer,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&lightingToFxaBarrier);

	const EnvironmentSettings environment = World::GetEnvironmentSettings();
	const bool useTaa = environment.aaTechnique == AATechnique::TAA;
	const bool useFxaa = environment.aaTechnique == AATechnique::FXAA;
	Pipeline& finalPipeline = useTaa
		? (ctx.taaHistoryInitialized ? *ctx.taaPipeline : *ctx.presentPipeline)
		: (useFxaa ? *ctx.fxaaPipeline : *ctx.presentPipeline);
	VkDescriptorSet finalDescriptorSet = (useTaa && ctx.taaHistoryInitialized)
		? ctx.taaDescriptorSets[frameInfo.imageIndex]
		: ctx.fxaaDescriptorSets[frameInfo.imageIndex];

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
	LightingRenderSystem::RecordFXAA(
		frameInfo.commandBuffer,
		frameInfo.swapChainExtent,
		finalPipeline,
		ctx.fxaaPipelineLayout,
		finalDescriptorSet);
	UiRenderSystem::Record(frameInfo.commandBuffer);
	vkCmdEndRenderPass(frameInfo.commandBuffer);

	if (useTaa)
	{
		VkImage swapchainImage = ctx.swapChainWrapper->getImage(static_cast<int>(frameInfo.imageIndex));
		const bool historyReady = ctx.taaHistoryInitialized;

		VkImageMemoryBarrier swapchainToTransfer{};
		swapchainToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		swapchainToTransfer.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		swapchainToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		swapchainToTransfer.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		swapchainToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		swapchainToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapchainToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapchainToTransfer.image = swapchainImage;
		swapchainToTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		swapchainToTransfer.subresourceRange.baseMipLevel = 0;
		swapchainToTransfer.subresourceRange.levelCount = 1;
		swapchainToTransfer.subresourceRange.baseArrayLayer = 0;
		swapchainToTransfer.subresourceRange.layerCount = 1;

		VkImageMemoryBarrier historyToTransfer{};
		historyToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		historyToTransfer.srcAccessMask = historyReady ? VK_ACCESS_SHADER_READ_BIT : 0;
		historyToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		historyToTransfer.oldLayout = historyReady ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
		historyToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		historyToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		historyToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		historyToTransfer.image = ctx.taaHistoryImage;
		historyToTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		historyToTransfer.subresourceRange.baseMipLevel = 0;
		historyToTransfer.subresourceRange.levelCount = 1;
		historyToTransfer.subresourceRange.baseArrayLayer = 0;
		historyToTransfer.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			frameInfo.commandBuffer,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&swapchainToTransfer);

		vkCmdPipelineBarrier(
			frameInfo.commandBuffer,
			historyReady ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&historyToTransfer);

		VkImageCopy copyRegion{};
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.mipLevel = 0;
		copyRegion.dstSubresource.baseArrayLayer = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.extent.width = frameInfo.swapChainExtent.width;
		copyRegion.extent.height = frameInfo.swapChainExtent.height;
		copyRegion.extent.depth = 1;

		vkCmdCopyImage(
			frameInfo.commandBuffer,
			swapchainImage,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			ctx.taaHistoryImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&copyRegion);

		VkImageMemoryBarrier historyToShader{};
		historyToShader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		historyToShader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		historyToShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		historyToShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		historyToShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		historyToShader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		historyToShader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		historyToShader.image = ctx.taaHistoryImage;
		historyToShader.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		historyToShader.subresourceRange.baseMipLevel = 0;
		historyToShader.subresourceRange.levelCount = 1;
		historyToShader.subresourceRange.baseArrayLayer = 0;
		historyToShader.subresourceRange.layerCount = 1;

		VkImageMemoryBarrier swapchainToPresent{};
		swapchainToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		swapchainToPresent.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		swapchainToPresent.dstAccessMask = 0;
		swapchainToPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		swapchainToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		swapchainToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapchainToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapchainToPresent.image = swapchainImage;
		swapchainToPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		swapchainToPresent.subresourceRange.baseMipLevel = 0;
		swapchainToPresent.subresourceRange.levelCount = 1;
		swapchainToPresent.subresourceRange.baseArrayLayer = 0;
		swapchainToPresent.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			frameInfo.commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			2,
			std::array<VkImageMemoryBarrier, 2>{historyToShader, swapchainToPresent}.data());

		ctx.taaHistoryInitialized = true;
	}
}

} // namespace LightingPass

} // namespace Piece
