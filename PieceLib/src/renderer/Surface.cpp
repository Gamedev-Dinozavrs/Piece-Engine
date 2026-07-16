#include <PiecePCH.h>

#include "Surface.h"
#include "VulkanContext.h"
#include <stdexcept>

namespace Piece {

Surface::Surface(VulkanContext& context, Window* window)
    : m_Context(context), m_Window(window) {
    createSurface();
}

Surface::~Surface() {
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_Context.instance(), surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
}

VkInstance Surface::instance() const {
    return m_Context.instance();
}

void Surface::createSurface() {
    if (!m_Window) {
        throw std::runtime_error("Surface requires a valid Window pointer");
    }

    VkResult result = glfwCreateWindowSurface(m_Context.instance(), static_cast<GLFWwindow*>(m_Window->GetNativeWindow()), nullptr, &surface_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

} // namespace Piece
