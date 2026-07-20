#pragma once

#include <renderer/Device.h>
#include <renderer/FrameResources.h>
#include <renderer/RenderPass.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <core/Core.h>
#include <vector>

namespace Piece {

class RenderPass;

class SwapChain {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    SwapChain(Device& deviceRef, VkExtent2D windowExtent);
    SwapChain(Device& deviceRef, VkExtent2D windowExtent, Ref<SwapChain> previous);
    ~SwapChain();

    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    VkFramebuffer getFrameBuffer(int index) { return m_SwapChainFramebuffers[index]; }
    VkRenderPass getRenderPass() { return m_renderPass->get(); }
    RenderPass& getRenderPassObject() { return *m_renderPass; }
    VkImageView getImageView(int index) { return m_SwapChainImageViews[index]; }
    VkImage getImage(int index) { return m_SwapChainImages[index]; }
    size_t imageCount() { return m_SwapChainImages.size(); }
    VkFormat getSwapChainImageFormat() { return m_SwapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return m_SwapChainExtent; }
    uint32_t width() { return m_SwapChainExtent.width; }
    uint32_t height() { return m_SwapChainExtent.height; }

    float extentAspectRatio() {
        return static_cast<float>(m_SwapChainExtent.width) / static_cast<float>(m_SwapChainExtent.height);
    }
    VkFormat findDepthFormat();

    VkResult acquireNextImage(uint32_t* imageIndex, const FrameResources& frameResources);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex, const FrameResources& frameResources, VkSemaphore renderFinishedSemaphore, std::vector<VkFence>& imagesInFlight);

    bool compareSwapFormats(const SwapChain& swapChain) const {
        return swapChain.m_SwapChainDepthFormat == m_SwapChainDepthFormat &&
            swapChain.m_SwapChainImageFormat == m_SwapChainImageFormat;
    }

private:
    void init();
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createFramebuffers();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VkFormat m_SwapChainImageFormat{VK_FORMAT_UNDEFINED};
    VkFormat m_SwapChainDepthFormat{VK_FORMAT_UNDEFINED};
    VkExtent2D m_SwapChainExtent{};

    std::vector<VkFramebuffer> m_SwapChainFramebuffers;
    Scope<RenderPass> m_renderPass;

    std::vector<VkImage> m_DepthImages;
    std::vector<VmaAllocation> m_DepthImageAllocations;
    std::vector<VkImageView> m_DepthImageViews;
    std::vector<VkImage> m_SwapChainImages;
    std::vector<VkImageView> m_SwapChainImageViews;

    Device& m_Device;
    VkExtent2D m_WindowExtent{};

    VkSwapchainKHR m_SwapChain{VK_NULL_HANDLE};
    Ref<SwapChain> m_OldSwapChain{nullptr};
};

} // namespace Piece
