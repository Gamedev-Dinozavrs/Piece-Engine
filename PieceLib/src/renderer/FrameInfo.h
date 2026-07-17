#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>

namespace Piece {

class EditorCamera;
class Mesh;
class Pipeline;
class Scene;

struct FrameInfo {
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    uint32_t imageIndex = 0;
    VkExtent2D swapChainExtent{};
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet globalDescriptorSet = VK_NULL_HANDLE;
    const std::unordered_map<uint32_t, VkDescriptorSet>* materialDescriptorSets = nullptr;

    Pipeline* pipeline = nullptr;
    EditorCamera* camera = nullptr;
    Scene* scene = nullptr;
};

} // namespace Piece