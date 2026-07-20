#include <PiecePCH.h>

#include "SwapChain.h"
#include <array>
#include <limits>
#include <algorithm>
#include <iostream>

namespace Piece {

SwapChain::SwapChain(Device& deviceRef, VkExtent2D extent)
    : m_Device{ deviceRef }, m_WindowExtent{ extent } {
    init();
}

SwapChain::SwapChain(Device& deviceRef, VkExtent2D extent, Ref<SwapChain> previous)
    : m_Device{ deviceRef }, m_WindowExtent{ extent }, m_OldSwapChain{ previous } {
    init();
    m_OldSwapChain = nullptr;
}

void SwapChain::init() {
    createSwapChain();
    createImageViews();
    m_renderPass = CreateScope<RenderPass>(m_Device, *this);
    createDepthResources();
    createFramebuffers();
}

SwapChain::~SwapChain() {
    for (auto imageView : m_SwapChainImageViews) {
        vkDestroyImageView(m_Device.device(), imageView, nullptr);
    }
    m_SwapChainImageViews.clear();

    if (m_SwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_Device.device(), m_SwapChain, nullptr);
        m_SwapChain = VK_NULL_HANDLE;
    }

    for (int i = 0; i < (int)m_DepthImages.size(); i++) {
        vkDestroyImageView(m_Device.device(), m_DepthImageViews[i], nullptr);
        if (m_DepthImageAllocations[i] != nullptr) {
            vmaDestroyImage(m_Device.allocator(), m_DepthImages[i], m_DepthImageAllocations[i]);
        } else {
            vkDestroyImage(m_Device.device(), m_DepthImages[i], nullptr);
        }
    }

    for (auto framebuffer : m_SwapChainFramebuffers) {
        vkDestroyFramebuffer(m_Device.device(), framebuffer, nullptr);
    }

    m_renderPass.reset();
}

VkResult SwapChain::acquireNextImage(uint32_t* imageIndex, const FrameResources& frameResources) {
    vkWaitForFences(m_Device.device(), 1, &frameResources.inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(
        m_Device.device(),
        m_SwapChain,
        std::numeric_limits<uint64_t>::max(),
        frameResources.imageAvailableSemaphore,
        VK_NULL_HANDLE,
        imageIndex);

    return result;
}

VkResult SwapChain::submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex, const FrameResources& frameResources, VkSemaphore renderFinishedSemaphore, std::vector<VkFence>& imagesInFlight) {
    if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_Device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[*imageIndex] = frameResources.inFlightFence;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { frameResources.imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(m_Device.device(), 1, &frameResources.inFlightFence);
    if (vkQueueSubmit(m_Device.graphicsQueue(), 1, &submitInfo, frameResources.inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_SwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = imageIndex;

    auto result = vkQueuePresentKHR(m_Device.presentQueue(), &presentInfo);

    return result;
}

void SwapChain::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = m_Device.getSwapChainSupport();

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Device.surface();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    Piece::QueueFamilyIndices indices = m_Device.findPhysicalQueueFamilies();
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = m_OldSwapChain == nullptr ? VK_NULL_HANDLE : m_OldSwapChain->m_SwapChain;

    if (vkCreateSwapchainKHR(m_Device.device(), &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(m_Device.device(), m_SwapChain, &imageCount, nullptr);
    m_SwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device.device(), m_SwapChain, &imageCount, m_SwapChainImages.data());

    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent = extent;
}

void SwapChain::createImageViews() {
    m_SwapChainImageViews.resize(m_SwapChainImages.size());
    for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_SwapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_SwapChainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_Device.device(), &viewInfo, nullptr, &m_SwapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
    }
}

void SwapChain::createFramebuffers() {
    m_SwapChainFramebuffers.resize(imageCount());
    for (size_t i = 0; i < imageCount(); i++) {
        std::array<VkImageView, 2> attachments = { m_SwapChainImageViews[i], m_DepthImageViews[i] };

        VkExtent2D swapChainExtent = getSwapChainExtent();
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass->get();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_Device.device(), &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void SwapChain::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    m_SwapChainDepthFormat = depthFormat;
    VkExtent2D swapChainExtent = getSwapChainExtent();

    m_DepthImages.resize(imageCount());
    m_DepthImageAllocations.resize(imageCount());
    m_DepthImageViews.resize(imageCount());

    for (int i = 0; i < (int)m_DepthImages.size(); i++) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;

        m_Device.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_DepthImages[i],
            m_DepthImageAllocations[i]);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_DepthImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_Device.device(), &viewInfo, nullptr, &m_DepthImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create depth image view!");
        }
    }
}

VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            PIECE_CORE_INFO("Present mode: Mailbox");
            return availablePresentMode;
        }
    }

    PIECE_CORE_INFO("Present mode: V-Sync");
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        VkExtent2D extent = capabilities.currentExtent;

        // Some platforms can transiently report 0x0 during minimize/fullscreen transitions.
        if (extent.width == 0 || extent.height == 0) {
            extent = m_WindowExtent;
        }

        extent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, extent.width));
        extent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, extent.height));

        extent.width = std::max(1u, extent.width);
        extent.height = std::max(1u, extent.height);
        return extent;
    }
    else {
        VkExtent2D actualExtent = m_WindowExtent;
        actualExtent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

        actualExtent.width = std::max(1u, actualExtent.width);
        actualExtent.height = std::max(1u, actualExtent.height);

        return actualExtent;
    }
}

VkFormat SwapChain::findDepthFormat() {
    return m_Device.findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

} // namespace Piece
