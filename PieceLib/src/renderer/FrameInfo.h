#pragma once

#include <vulkan/vulkan.h>

namespace Piece {

class EditorCamera;
class Mesh;
class Pipeline;
class RenderObject;

struct FrameInfo {
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    uint32_t imageIndex = 0;
    VkExtent2D swapChainExtent{};
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    Pipeline* pipeline = nullptr;
    Mesh* mesh = nullptr;
    EditorCamera* camera = nullptr;
    RenderObject* renderObject = nullptr;
};

} // namespace Piece