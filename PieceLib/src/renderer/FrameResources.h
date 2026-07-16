#pragma once

#include <vulkan/vulkan.h>

namespace Piece {

struct FrameResources {
    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    VkSemaphore imageAvailableSemaphore{VK_NULL_HANDLE};
    VkSemaphore renderFinishedSemaphore{VK_NULL_HANDLE};
    VkFence inFlightFence{VK_NULL_HANDLE};
    uint32_t imageIndex{0};
};

} // namespace Piece
