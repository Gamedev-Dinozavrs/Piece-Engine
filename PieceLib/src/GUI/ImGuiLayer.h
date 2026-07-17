#pragma once

#include <layer/Layer.h>
#include <event/Event.h>

#include <vulkan/vulkan.h>
#include <memory>

namespace Piece {

	class Device;
	class RenderPass;
	class VulkanContext;

	class ImGuiLayer : public Layer {
	public:
		ImGuiLayer(Device& device, RenderPass& renderPass, VulkanContext& context);
		~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& event) override;

		void begin();
		void end();

		void blockEvents(bool block) { m_blockEvents = block; }
		void setDarkThemeColors();

		// Call this during render pass to record ImGui draw commands
		void Begin();
		void End();
		void RecordCommandBuffer(VkCommandBuffer commandBuffer);
		void OnSwapChainRecreated(RenderPass& renderPass);

	private:
		bool m_blockEvents = true;
		
		// Vulkan resources for ImGui
		VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
		
		Device* m_device = nullptr;
		RenderPass* m_renderPass = nullptr;
		VulkanContext* m_vulkanContext = nullptr;
	};

} // namespace Piece
