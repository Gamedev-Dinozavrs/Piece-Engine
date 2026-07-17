#pragma once

#include <renderer/FrameResources.h>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <memory>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <core/Core.h>

namespace Piece {

class Window;
class VulkanContext;
class Surface;
class Device;
class SwapChain;
class Pipeline;
class Mesh;
class EditorCamera;
class Scene;
class DescriptorSetLayout;
class DescriptorPool;
class Texture;
class ShaderLibrary;
class Buffer;

struct OffscreenFrameResources {
    VkImage msaaWorldPosRoughnessImage{VK_NULL_HANDLE};
    VmaAllocation msaaWorldPosRoughnessAllocation{nullptr};
    VkImageView msaaWorldPosRoughnessImageView{VK_NULL_HANDLE};

    VkImage msaaAlbedoAoImage{VK_NULL_HANDLE};
    VmaAllocation msaaAlbedoAoAllocation{nullptr};
    VkImageView msaaAlbedoAoImageView{VK_NULL_HANDLE};

    VkImage msaaNormalAoImage{VK_NULL_HANDLE};
    VmaAllocation msaaNormalAoAllocation{nullptr};
    VkImageView msaaNormalAoImageView{VK_NULL_HANDLE};

    VkImage depthImage{VK_NULL_HANDLE};
    VmaAllocation depthAllocation{nullptr};
    VkImageView depthImageView{VK_NULL_HANDLE};

    // Resolve images for MSAA
    VkImage worldPosRoughnessImage{VK_NULL_HANDLE};
    VmaAllocation worldPosRoughnessAllocation{nullptr};
    VkImageView worldPosRoughnessImageView{VK_NULL_HANDLE};

    VkImage albedoAoImage{VK_NULL_HANDLE};
    VmaAllocation albedoAoAllocation{nullptr};
    VkImageView albedoAoImageView{VK_NULL_HANDLE};

    VkImage normalAoImage{VK_NULL_HANDLE};
    VmaAllocation normalAoAllocation{nullptr};
    VkImageView normalAoImageView{VK_NULL_HANDLE};

    VkFramebuffer framebuffer{VK_NULL_HANDLE};
};

struct RendererContext {
    Window* window{nullptr};

    std::unique_ptr<VulkanContext> vulkanContext;
    std::unique_ptr<Surface> surfaceWrapper;
    std::unique_ptr<Device> deviceWrapper;
    std::unique_ptr<SwapChain> swapChainWrapper;
    std::unique_ptr<Pipeline> geometryPipeline;
    std::unique_ptr<Pipeline> lightingPipeline;

    std::shared_ptr<Mesh> quadMesh;
    std::shared_ptr<Mesh> cubeMesh;
    std::shared_ptr<Mesh> sphereMesh;
    std::shared_ptr<EditorCamera> camera;
    Ref<Scene> scene;

    std::unique_ptr<DescriptorSetLayout> materialSetLayout;
    std::unique_ptr<DescriptorPool> materialDescriptorPool;
    std::unique_ptr<DescriptorSetLayout> globalSetLayout;
    std::unique_ptr<DescriptorPool> globalDescriptorPool;
    std::unique_ptr<DescriptorSetLayout> compositeSetLayout;
    std::unique_ptr<DescriptorPool> compositeDescriptorPool;
    std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;
    std::unordered_map<uint32_t, VkDescriptorSet> objectMaterialDescriptors;
    std::unordered_map<uint32_t, std::string> objectBoundMaterialSignature;
    std::vector<std::unique_ptr<Buffer>> globalUboBuffers;
    std::vector<VkDescriptorSet> globalDescriptorSets;
    std::vector<VkDescriptorSet> compositeDescriptorSets;

    std::unique_ptr<ShaderLibrary> shaderLibrary;

    VkSurfaceKHR surface{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    VkQueue graphicsQueue{VK_NULL_HANDLE};
    VkQueue presentQueue{VK_NULL_HANDLE};

    VkFormat swapChainImageFormat{VK_FORMAT_UNDEFINED};
    VkExtent2D swapChainExtent{};
    VkSampleCountFlagBits msaaSamples{VK_SAMPLE_COUNT_1_BIT};
    VkRenderPass lightingRenderPass{VK_NULL_HANDLE};
    VkRenderPass geometryRenderPass{VK_NULL_HANDLE};
    VkSampler compositeSampler{VK_NULL_HANDLE};
    VkFormat offscreenWorldPosRoughnessFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    VkFormat offscreenAlbedoAoFormat{VK_FORMAT_R8G8B8A8_UNORM};
    std::vector<OffscreenFrameResources> offscreenFrames;

    VkCommandPool commandPool{VK_NULL_HANDLE};
    VkPipelineLayout geometryPipelineLayout{VK_NULL_HANDLE};
    VkPipelineLayout lightingPipelineLayout{VK_NULL_HANDLE};

    std::vector<FrameResources> frameResources;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame{0};

};

} // namespace Piece
