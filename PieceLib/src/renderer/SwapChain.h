#pragma once

#include <renderer/Device.h>
#include <renderer/FrameResources.h>
#include <renderer/RenderPass.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <memory>
#include <vector>

namespace Piece {

class RenderPass;

class SwapChain {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    SwapChain(Device& deviceRef, VkExtent2D windowExtent);
    SwapChain(Device& deviceRef, VkExtent2D windowExtent, std::shared_ptr<SwapChain> previous);
    ~SwapChain();

    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
    VkRenderPass getRenderPass() { return m_renderPass->get(); }
    RenderPass& getRenderPassObject() { return *m_renderPass; }
    VkImageView getImageView(int index) { return swapChainImageViews[index]; }
    size_t imageCount() { return swapChainImages.size(); }
    VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return swapChainExtent; }
    uint32_t width() { return swapChainExtent.width; }
    uint32_t height() { return swapChainExtent.height; }

    float extentAspectRatio() {
        return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
    }
    VkFormat findDepthFormat();

    VkResult acquireNextImage(uint32_t* imageIndex, const FrameResources& frameResources);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex, const FrameResources& frameResources, std::vector<VkFence>& imagesInFlight);

    bool compareSwapFormats(const SwapChain& swapChain) const {
        return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
            swapChain.swapChainImageFormat == swapChainImageFormat;
    }

private:
    void init();
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createFramebuffers();

    // Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VkFormat swapChainImageFormat{VK_FORMAT_UNDEFINED};
    VkFormat swapChainDepthFormat{VK_FORMAT_UNDEFINED};
    VkExtent2D swapChainExtent{};

    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::unique_ptr<RenderPass> m_renderPass;

    std::vector<VkImage> depthImages;
    std::vector<VmaAllocation> depthImageAllocations;
    std::vector<VkImageView> depthImageViews;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    Device& device;
    VkExtent2D windowExtent{};

    VkSwapchainKHR swapChain{VK_NULL_HANDLE};
    std::shared_ptr<SwapChain> oldSwapChain{nullptr};
};

} // namespace Piece
