#include <PiecePCH.h>

#include "ImGuiLayer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include <ImGuizmo.h>

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
		ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
		ImGuizmo::GetStyle().HatchedAxisLineThickness = 0.0f;
		ImGuiIO& io = ImGui::GetIO();
		
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// Load the bundled editor font so startup never falls back to ImGui's bitmap font.
		const std::filesystem::path fontPath = "Dependencies/imgui/misc/fonts/Karla-Regular.ttf";
		if (std::filesystem::exists(fontPath)) {
			io.FontDefault = io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 16.0f);
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
		ImGuizmo::BeginFrame();
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
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(8.0f, 7.0f);
		style.FramePadding = ImVec2(7.0f, 4.0f);
		style.CellPadding = ImVec2(7.0f, 4.0f);
		style.ItemSpacing = ImVec2(7.0f, 5.0f);
		style.ItemInnerSpacing = ImVec2(5.0f, 4.0f);
		style.IndentSpacing = 18.0f;
		style.ScrollbarSize = 12.0f;
		style.GrabMinSize = 8.0f;
		style.WindowRounding = 3.0f;
		style.ChildRounding = 3.0f;
		style.FrameRounding = 3.0f;
		style.PopupRounding = 4.0f;
		style.ScrollbarRounding = 6.0f;
		style.GrabRounding = 3.0f;
		style.TabRounding = 3.0f;
		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 0.0f;
		style.TabBorderSize = 0.0f;
		style.SeparatorTextBorderSize = 1.0f;

		auto& colors = style.Colors;
		colors[ImGuiCol_Text] = ImVec4(0.88f, 0.91f, 0.94f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.48f, 0.55f, 0.63f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.075f, 0.105f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.065f, 0.088f, 0.120f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.075f, 0.098f, 0.132f, 0.99f);
		colors[ImGuiCol_Border] = ImVec4(0.16f, 0.21f, 0.28f, 1.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.01f, 0.02f, 0.03f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.095f, 0.125f, 0.165f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.18f, 0.23f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.22f, 0.28f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.045f, 0.060f, 0.085f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.075f, 0.100f, 0.135f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.045f, 0.060f, 0.085f, 0.90f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.040f, 0.055f, 0.078f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.040f, 0.055f, 0.075f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.23f, 0.30f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.26f, 0.32f, 0.40f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.92f, 0.47f, 0.16f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.55f, 0.20f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.92f, 0.45f, 0.14f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.62f, 0.26f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.12f, 0.16f, 0.21f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.78f, 0.35f, 0.10f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.96f, 0.48f, 0.14f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.11f, 0.15f, 0.20f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.72f, 0.31f, 0.09f, 0.92f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.94f, 0.45f, 0.12f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.20f, 0.27f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.88f, 0.40f, 0.11f, 1.00f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(1.00f, 0.53f, 0.17f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.89f, 0.40f, 0.11f, 0.24f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.94f, 0.45f, 0.13f, 0.70f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.54f, 0.18f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.065f, 0.085f, 0.115f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.58f, 0.25f, 0.08f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.83f, 0.37f, 0.10f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.050f, 0.068f, 0.095f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.16f, 0.15f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.96f, 0.46f, 0.13f, 0.65f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.035f, 0.050f, 0.070f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.46f, 0.57f, 0.69f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.56f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.91f, 0.42f, 0.12f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.62f, 0.25f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.085f, 0.115f, 0.155f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.16f, 0.21f, 0.28f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.11f, 0.15f, 0.20f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.10f, 0.13f, 0.17f, 0.38f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.83f, 0.37f, 0.10f, 0.45f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 0.58f, 0.20f, 0.95f);
		colors[ImGuiCol_NavCursor] = ImVec4(1.00f, 0.55f, 0.18f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.55f, 0.18f, 0.70f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.01f, 0.02f, 0.04f, 0.72f);
	}

} // namespace Piece
