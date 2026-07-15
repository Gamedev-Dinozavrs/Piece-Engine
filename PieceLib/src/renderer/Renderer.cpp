#include <renderer/Renderer.h>
#include <renderer/Device.h>
#include <renderer/FrameInfo.h>
#include <renderer/Surface.h>
#include <renderer/SwapChain.h>
#include <renderer/Pipeline.h>
#include <renderer/VulkanContext.h>
#include <scene/EditorCamera.h>
#include <scene/Mesh.h>
#include <scene/RenderObject.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#ifndef PIECE_SHADER_DIR
#define PIECE_SHADER_DIR "./PieceLib/src/shaders"
#endif

namespace Piece {

    namespace {
        Window* s_Window = nullptr;
        VulkanContext* s_VulkanContext = nullptr;
        Surface* s_SurfaceWrapper = nullptr;
        Device* s_DeviceWrapper = nullptr;

        VkSurfaceKHR s_Surface = VK_NULL_HANDLE;
        VkPhysicalDevice s_PhysicalDevice = VK_NULL_HANDLE; // owned by Device
        VkDevice s_Device = VK_NULL_HANDLE; // owned by Device
        VkQueue s_GraphicsQueue = VK_NULL_HANDLE; // owned by Device
        VkQueue s_PresentQueue = VK_NULL_HANDLE; // owned by Device

        VkSwapchainKHR s_SwapChain = VK_NULL_HANDLE;
        std::vector<VkImage> s_SwapChainImages;
        VkFormat s_SwapChainImageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D s_SwapChainExtent = {};
        std::vector<VkImageView> s_SwapChainImageViews;

        VkRenderPass s_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> s_SwapChainFramebuffers;

        SwapChain* s_SwapChainWrapper = nullptr;
        Pipeline* s_PipelineWrapper = nullptr;
        std::shared_ptr<Mesh> s_Mesh = nullptr;
        std::shared_ptr<EditorCamera> s_Camera = nullptr;
        std::shared_ptr<RenderObject> s_RenderObject = nullptr;

        VkCommandPool s_CommandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> s_CommandBuffers;

        VkSemaphore s_ImageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore s_RenderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence s_InFlightFence = VK_NULL_HANDLE;
        VkPipelineLayout s_PipelineLayout = VK_NULL_HANDLE;
        VkPipeline s_GraphicsPipeline = VK_NULL_HANDLE;

        float s_RotationY = 0.0f;

        const std::vector<const char*> s_DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        // Use Piece::QueueFamilyIndices and Piece::SwapChainSupportDetails from Device.h

        Piece::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
            Piece::QueueFamilyIndices indices;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies) {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.graphicsFamily = i;
                }

                VkBool32 presentSupport = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, s_Surface, &presentSupport);

                if (presentSupport) {
                    indices.presentFamily = i;
                }

                if (indices.isComplete()) {
                    break;
                }

                i++;
            }

            return indices;
        }

        Piece::SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) {
            Piece::SwapChainSupportDetails details;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, s_Surface, &details.capabilities);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, s_Surface, &formatCount, nullptr);
            if (formatCount != 0) {
                details.formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, s_Surface, &formatCount, details.formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, s_Surface, &presentModeCount, nullptr);
            if (presentModeCount != 0) {
                details.presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, s_Surface, &presentModeCount, details.presentModes.data());
            }

            return details;
        }

        bool HasRequiredDeviceExtensions(VkPhysicalDevice device) {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions(s_DeviceExtensions.begin(), s_DeviceExtensions.end());
            for (const auto& extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
            }

            return requiredExtensions.empty();
        }

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
            for (const auto& availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }
            return availableFormats[0];
        }

        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
            for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentMode;
                }
            }
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
            if (capabilities.currentExtent.width != UINT32_MAX) {
                return capabilities.currentExtent;
            }

            VkExtent2D actualExtent = { s_Window->GetWidth(), s_Window->GetHeight() };

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }

        std::vector<Vertex> CreateCubeVertices() {
            return {
                {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
                {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
                {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
                {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
                {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
                {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
                {{ 0.5f,  0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}},
                {{-0.5f,  0.5f, -0.5f}, {0.8f, 0.2f, 0.2f}}
            };
        }

        std::vector<uint32_t> CreateCubeIndices() {
            return {
                0, 1, 2, 2, 3, 0,
                4, 6, 5, 4, 7, 6,
                3, 2, 6, 6, 7, 3,
                4, 5, 1, 1, 0, 4,
                1, 5, 6, 6, 2, 1,
                4, 0, 3, 3, 7, 4
            };
        }

        FrameInfo BuildFrameInfo(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
            FrameInfo frameInfo{};
            frameInfo.commandBuffer = commandBuffer;
            frameInfo.imageIndex = imageIndex;
            frameInfo.swapChainExtent = s_SwapChainExtent;
            frameInfo.renderPass = s_RenderPass;
            frameInfo.framebuffer = s_SwapChainWrapper->getFrameBuffer(imageIndex);
            frameInfo.pipelineLayout = s_PipelineLayout;
            frameInfo.pipeline = s_PipelineWrapper;
            frameInfo.mesh = s_Mesh.get();
            frameInfo.camera = s_Camera.get();
            frameInfo.renderObject = s_RenderObject.get();
            return frameInfo;
        }

        void RecordCommandBuffer(const FrameInfo& frameInfo) {
            VkCommandBuffer commandBuffer = frameInfo.commandBuffer;
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            VkClearValue clearValues[2] = {};
            clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
            clearValues[1].depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = frameInfo.renderPass;
            renderPassInfo.framebuffer = frameInfo.framebuffer;
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = frameInfo.swapChainExtent;
            renderPassInfo.clearValueCount = 2;
            renderPassInfo.pClearValues = clearValues;

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            frameInfo.pipeline->bind(commandBuffer);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(frameInfo.swapChainExtent.width);
            viewport.height = static_cast<float>(frameInfo.swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = frameInfo.swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            if (frameInfo.renderObject && frameInfo.mesh && frameInfo.camera) {
                glm::mat4 model = frameInfo.renderObject->modelMatrix();
                glm::mat4 view = frameInfo.camera->view();
                glm::mat4 proj = frameInfo.camera->projection();
                glm::mat4 mvp = proj * view * model;
                vkCmdPushConstants(commandBuffer, frameInfo.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);

                frameInfo.mesh->bind(commandBuffer);
                frameInfo.mesh->draw(commandBuffer);
            }

            vkCmdEndRenderPass(commandBuffer);

            vkEndCommandBuffer(commandBuffer);
        }

        static void CreateGraphicsPipeline() {
            PipelineConfigInfo configInfo{};
            Pipeline::defaultPipelineConfigInfo(configInfo);
            configInfo.renderPass = s_RenderPass;
            configInfo.pipelineLayout = s_PipelineLayout;

            s_PipelineWrapper = new Pipeline(
                *s_DeviceWrapper,
                PIECE_SHADER_DIR "/textured.vert.spv",
                PIECE_SHADER_DIR "/textured.frag.spv",
                configInfo);
        }
    }

    void Renderer::Init(Window* window) {
        assert(window && "Window must not be null");
        s_Window = window;
        s_VulkanContext = new VulkanContext();
        s_SurfaceWrapper = new Surface(*s_VulkanContext, window);
        s_DeviceWrapper = new Device(*s_SurfaceWrapper, *s_VulkanContext);

        s_Device = s_DeviceWrapper->device();
        s_PhysicalDevice = s_DeviceWrapper->getPhysicalDevice();
        s_Surface = s_SurfaceWrapper->surface();
        s_GraphicsQueue = s_DeviceWrapper->graphicsQueue();
        s_PresentQueue = s_DeviceWrapper->presentQueue();

        // create swapchain wrapper which also creates image views, render pass, framebuffers and sync
        VkExtent2D extent{ (uint32_t)s_Window->GetWidth(), (uint32_t)s_Window->GetHeight() };
        s_SwapChainWrapper = new SwapChain(*s_DeviceWrapper, extent);

        s_SwapChainImageFormat = s_SwapChainWrapper->getSwapChainImageFormat();
        s_SwapChainExtent = s_SwapChainWrapper->getSwapChainExtent();
        s_RenderPass = s_SwapChainWrapper->getRenderPass();

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(glm::mat4);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        VkResult result = vkCreatePipelineLayout(s_Device, &pipelineLayoutInfo, nullptr, &s_PipelineLayout);
        assert(result == VK_SUCCESS);

        auto cubeVertices = CreateCubeVertices();
        auto cubeIndices = CreateCubeIndices();
        s_Mesh = std::make_shared<Mesh>(*s_DeviceWrapper, cubeVertices, cubeIndices);
        s_Camera = std::make_shared<EditorCamera>(70.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 100.0f);
        s_RenderObject = std::make_shared<RenderObject>(s_Mesh, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));

        CreateGraphicsPipeline();
        // command pool is created by Device; get it
        s_CommandPool = s_DeviceWrapper->getCommandPool();
        CreateCommandBuffers();
    }

    void Renderer::Shutdown() {
        if (s_Device == VK_NULL_HANDLE) {
            return;
        }

        vkDeviceWaitIdle(s_Device);

        s_RenderObject.reset();
        s_Mesh.reset();
        s_Camera.reset();

        if (s_PipelineWrapper) {
            delete s_PipelineWrapper;
            s_PipelineWrapper = nullptr;
        }
        if (s_PipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(s_Device, s_PipelineLayout, nullptr);
            s_PipelineLayout = VK_NULL_HANDLE;
        }

        if (s_ImageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(s_Device, s_ImageAvailableSemaphore, nullptr);
            s_ImageAvailableSemaphore = VK_NULL_HANDLE;
        }
        if (s_RenderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(s_Device, s_RenderFinishedSemaphore, nullptr);
            s_RenderFinishedSemaphore = VK_NULL_HANDLE;
        }
        if (s_InFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(s_Device, s_InFlightFence, nullptr);
            s_InFlightFence = VK_NULL_HANDLE;
        }

        CleanupSwapChain();

        if (s_SwapChainWrapper) {
            delete s_SwapChainWrapper;
            s_SwapChainWrapper = nullptr;
        }

        if (s_DeviceWrapper) {
            delete s_DeviceWrapper;
            s_DeviceWrapper = nullptr;
        }
        if (s_SurfaceWrapper) {
            delete s_SurfaceWrapper;
            s_SurfaceWrapper = nullptr;
        }
        if (s_VulkanContext) {
            delete s_VulkanContext;
            s_VulkanContext = nullptr;
        }

        s_Window = nullptr;
        s_Device = VK_NULL_HANDLE;
        s_PhysicalDevice = VK_NULL_HANDLE;
        s_Surface = VK_NULL_HANDLE;
        s_GraphicsQueue = VK_NULL_HANDLE;
        s_PresentQueue = VK_NULL_HANDLE;
    }

    void Renderer::DrawFrame() {
        uint32_t imageIndex;
        VkResult result = s_SwapChainWrapper->acquireNextImage(&imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapChain();
            return;
        }
        assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

        vkResetCommandBuffer(s_CommandBuffers[imageIndex], 0);
    FrameInfo frameInfo = BuildFrameInfo(s_CommandBuffers[imageIndex], imageIndex);
    RecordCommandBuffer(frameInfo);

        result = s_SwapChainWrapper->submitCommandBuffers(&s_CommandBuffers[imageIndex], &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            RecreateSwapChain();
        }
        assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);
    }

    void Renderer::Update(Timestep ts) {
        if (s_Camera) {
            s_Camera->onUpdate(static_cast<float>(ts));
        }

        if (s_RenderObject) {
            const float speed = 45.0f; // degrees per second
            s_RotationY += speed * static_cast<float>(ts);
            if (s_RotationY >= 360.0f) s_RotationY -= 360.0f;
            s_RenderObject->setRotation(glm::vec3(0.0f, s_RotationY, 0.0f));
        }
    }

    void Renderer::OnWindowResize(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) {
            return;
        }

        if (s_Camera) {
            s_Camera->setViewportSize(static_cast<float>(width), static_cast<float>(height));
        }

        RecreateSwapChain();
    }

    bool Renderer::OnMouseScrolled(MouseScrolledEvent& event) {
        if (s_Camera) {
            s_Camera->onMouseScroll(event.GetOffsetY());
        }
        return false;
    }

    /* Command pool is created by Device; Renderer uses Device::getCommandPool() */

    void Renderer::CreateCommandBuffers() {
        size_t count = s_SwapChainWrapper ? s_SwapChainWrapper->imageCount() : 1;
        s_CommandBuffers.resize(count);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = s_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(s_CommandBuffers.size());

        VkResult result = vkAllocateCommandBuffers(s_Device, &allocInfo, s_CommandBuffers.data());
        assert(result == VK_SUCCESS);

        for (size_t i = 0; i < s_CommandBuffers.size(); i++) {
            FrameInfo frameInfo = BuildFrameInfo(s_CommandBuffers[i], static_cast<uint32_t>(i));
            RecordCommandBuffer(frameInfo);
        }
    }

    void Renderer::CreateSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkResult result = vkCreateSemaphore(s_Device, &semaphoreInfo, nullptr, &s_ImageAvailableSemaphore);
        assert(result == VK_SUCCESS);
        result = vkCreateSemaphore(s_Device, &semaphoreInfo, nullptr, &s_RenderFinishedSemaphore);
        assert(result == VK_SUCCESS);
        result = vkCreateFence(s_Device, &fenceInfo, nullptr, &s_InFlightFence);
        assert(result == VK_SUCCESS);
    }

    void Renderer::CleanupSwapChain() {
        // Command buffers belong to renderer
        if (!s_CommandBuffers.empty()) {
            vkFreeCommandBuffers(s_Device, s_CommandPool, static_cast<uint32_t>(s_CommandBuffers.size()), s_CommandBuffers.data());
            s_CommandBuffers.clear();
        }
    }

    void Renderer::RecreateSwapChain() {
        vkDeviceWaitIdle(s_Device);

        CleanupSwapChain();

        if (s_SwapChainWrapper) {
            delete s_SwapChainWrapper;
            s_SwapChainWrapper = nullptr;
        }

        VkExtent2D extent{ (uint32_t)s_Window->GetWidth(), (uint32_t)s_Window->GetHeight() };
        s_SwapChainWrapper = new SwapChain(*s_DeviceWrapper, extent);

        s_SwapChainImageFormat = s_SwapChainWrapper->getSwapChainImageFormat();
        s_SwapChainExtent = s_SwapChainWrapper->getSwapChainExtent();
        s_RenderPass = s_SwapChainWrapper->getRenderPass();

        if (s_PipelineWrapper) {
            delete s_PipelineWrapper;
            s_PipelineWrapper = nullptr;
        }

        CreateGraphicsPipeline();
        CreateCommandBuffers();
    }

} // namespace Piece
