#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Piece {

class VulkanContext {
public:
#ifdef NDEBUG
    static constexpr bool enableValidationLayers = false;
#else
    static constexpr bool enableValidationLayers = true;
#endif

    VulkanContext();
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    VkInstance instance() const { return instance_; }
    bool validationEnabled() const { return enableValidationLayers; }

private:
    void createInstance();
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const;
    bool checkValidationLayerSupport() const;
    std::vector<const char*> getRequiredExtensions() const;

    VkInstance instance_{VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT debugMessenger_{VK_NULL_HANDLE};
    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

} // namespace Piece
