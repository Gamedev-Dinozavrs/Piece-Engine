#pragma once

#include <vulkan/vulkan.h>

namespace Piece {

class Device;
class SwapChain;

class RenderPass {
public:
	RenderPass(Device& device, SwapChain& swapChain);
	~RenderPass();

	// Not copyable or movable
	RenderPass(const RenderPass&) = delete;
	RenderPass& operator=(const RenderPass&) = delete;
	RenderPass(RenderPass&&) = delete;
	RenderPass& operator=(RenderPass&&) = delete;

	VkRenderPass get() const { return m_renderPass; }
	operator VkRenderPass() const { return m_renderPass; }

private:
	void create();

	Device& m_device;
	SwapChain& m_swapChain;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
};

} // namespace Piece
