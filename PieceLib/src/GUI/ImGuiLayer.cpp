#include <PiecePCH.h>

#include "ImGuiLayer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <renderer/Device.h>
#include <renderer/RenderPass.h>
#include <renderer/VulkanContext.h>
#include <core/Application.h>
#include <core/Input.h>

#include <filesystem>

#include <GLFW/glfw3.h>

namespace Piece {

	ImGuiLayer::ImGuiLayer(Device& device, RenderPass& renderPass, VulkanContext& context) 
		: Layer("ImGui Layer"), m_device(&device), m_renderPass(&renderPass), m_vulkanContext(&context) {
	}

	ImGuiLayer::~ImGuiLayer() {
	}

	void ImGuiLayer::OnAttach() {
		// Setup ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// Load font
		const std::filesystem::path fontPath = "assets/fonts/arial.ttf";
		if (std::filesystem::exists(fontPath)) {
			io.FontDefault = io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 18.0f);
		}
		if (io.FontDefault == nullptr) {
			io.FontDefault = io.Fonts->AddFontDefault();
		}

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		setDarkThemeColors();

		// Get window and Vulkan objects
		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
		
		// Get SwapChain for render pass
		// TODO: Store SwapChain reference in Renderer or pass via constructor
		// For now, we need to retrieve it from somewhere accessible
		
		// Create descriptor pool for ImGui
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		
		if (vkCreateDescriptorPool(m_device->device(), &pool_info, nullptr, &m_imguiDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create ImGui descriptor pool");
		}

		// Setup ImGui GLFW backend (for input)
		ImGui_ImplGlfw_InitForVulkan(window, true);

		// Setup ImGui Vulkan backend
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = m_vulkanContext->instance();
		init_info.PhysicalDevice = m_device->getPhysicalDevice();
		init_info.Device = m_device->device();
		init_info.QueueFamily = m_device->findPhysicalQueueFamilies().graphicsFamily;
		init_info.Queue = m_device->graphicsQueue();
		init_info.DescriptorPool = m_imguiDescriptorPool;
		init_info.MinImageCount = 2;
		init_info.ImageCount = 2;

		// Setup pipeline info with render pass
		init_info.PipelineInfoMain.RenderPass = m_renderPass->get();
		init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		
		ImGui_ImplVulkan_Init(&init_info);
	}

	void ImGuiLayer::OnDetach() {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		if (m_imguiDescriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(m_device->device(), m_imguiDescriptorPool, nullptr);
			m_imguiDescriptorPool = VK_NULL_HANDLE;
		}
	}

	void ImGuiLayer::OnEvent(Event& event) {
		if (m_blockEvents) {
			ImGuiIO& io = ImGui::GetIO();
			event.IsHandled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			event.IsHandled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::Begin() {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::End() {
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		ImGui::Render();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void ImGuiLayer::RecordCommandBuffer(VkCommandBuffer commandBuffer) {
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

	void ImGuiLayer::OnSwapChainRecreated(RenderPass& renderPass) {
		m_renderPass = &renderPass;

		if (ImGui::GetCurrentContext() == nullptr) {
			return;
		}

		ImGui_ImplVulkan_PipelineInfo pipelineInfo{};
		pipelineInfo.RenderPass = m_renderPass->get();
		pipelineInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		ImGui_ImplVulkan_CreateMainPipeline(&pipelineInfo);
	}

	void ImGuiLayer::setDarkThemeColors() {
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.11f, 0.105f, 0.12f, 1.0f };

		// headers
		colors[ImGuiCol_Header] = ImVec4{ 0.3f, 0.405f, 0.511f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.1f, 0.405f, 0.811f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.3f, 0.505f, 0.511f, 1.0f };

		// buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.305f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.405f, 0.31f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.4f, 0.505f, 0.41f, 1.0f };

		// frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.1f, 0.305f, 0.311f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.311f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.3f, 0.305f, 0.511f, 1.0f };

		// tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.4f, 0.105f, 0.451f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.15f, 0.105f, 0.211f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.55f, 0.505f, 0.531f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.21f, 0.105f, 0.11f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.1f, 0.105f, 0.311f, 1.0f };

		// title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.45f, 0.3505f, 0.351f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.38f, 0.3595f, 0.431f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.217f, 0.2205f, 0.291f, 1.0f };
	}

} // namespace Piece
