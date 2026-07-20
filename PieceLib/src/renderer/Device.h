#pragma once

#include <renderer/Surface.h>
#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace Piece {
class VulkanContext;

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    uint32_t graphicsFamily = UINT32_MAX;
    uint32_t presentFamily = UINT32_MAX;
    bool graphicsFamilyHasValue = false;
    bool presentFamilyHasValue = false;
    bool isComplete() const { return graphicsFamilyHasValue && presentFamilyHasValue; }
};

class Device {
public:
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    Device(Surface& surface, VulkanContext& context);
    ~Device();

    // Not copyable or movable
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    VkCommandPool getCommandPool() const { return m_CommandPool; }
    VkDevice device() const { return device_; }
    VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; }
    VkSurfaceKHR surface() const { return m_Surface.surface(); }
    VkQueue graphicsQueue() const { return graphicsQueue_; }
    VkQueue presentQueue() const { return presentQueue_; }
    bool isSampleRateShadingEnabled() const { return sampleRateShadingEnabled_; }

    SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(m_PhysicalDevice); }
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(m_PhysicalDevice); }
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VmaAllocator allocator() const { return allocator_; }

    // Buffer Helper Functions
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation& allocation);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

    void createImageWithInfo(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VmaAllocation& allocation);

    VkPhysicalDeviceProperties m_PhysicalDeviceProperties;

private:
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();

    // helper functions
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    Surface& m_Surface;
    VulkanContext& m_Context;
    VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};
    VkCommandPool m_CommandPool{VK_NULL_HANDLE};
    VmaAllocator allocator_{nullptr};

    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};
    bool sampleRateShadingEnabled_{false};

    const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};

} // namespace Piece
