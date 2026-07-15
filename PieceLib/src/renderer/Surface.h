#pragma once

#include <window/Window.h>
#include <vulkan/vulkan.h>

namespace Piece {

class VulkanContext;

class Surface {
public:
    Surface(VulkanContext& context, Window* window);
    ~Surface();

    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;
    Surface(Surface&&) = delete;
    Surface& operator=(Surface&&) = delete;

    VkSurfaceKHR surface() const { return surface_; }
    VkInstance instance() const;

private:
    void createSurface();

    VulkanContext& m_Context;
    Window* m_Window;
    VkSurfaceKHR surface_{VK_NULL_HANDLE};
};

} // namespace Piece
