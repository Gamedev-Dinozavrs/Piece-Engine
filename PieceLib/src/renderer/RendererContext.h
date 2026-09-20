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

    VkImage msaaEmissiveImage{VK_NULL_HANDLE};
    VmaAllocation msaaEmissiveAllocation{nullptr};
    VkImageView msaaEmissiveImageView{VK_NULL_HANDLE};

    VkImage msaaBloomParamsImage{VK_NULL_HANDLE};
    VmaAllocation msaaBloomParamsAllocation{nullptr};
    VkImageView msaaBloomParamsImageView{VK_NULL_HANDLE};

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

    VkImage emissiveImage{VK_NULL_HANDLE};
    VmaAllocation emissiveAllocation{nullptr};
    VkImageView emissiveImageView{VK_NULL_HANDLE};

    VkImage bloomParamsImage{VK_NULL_HANDLE};
    VmaAllocation bloomParamsAllocation{nullptr};
    VkImageView bloomParamsImageView{VK_NULL_HANDLE};

    VkImage msaaEntityIdImage{VK_NULL_HANDLE};
    VmaAllocation msaaEntityIdAllocation{nullptr};
    VkImageView msaaEntityIdImageView{VK_NULL_HANDLE};

    VkImage entityIdImage{VK_NULL_HANDLE};
    VmaAllocation entityIdAllocation{nullptr};
    VkImageView entityIdImageView{VK_NULL_HANDLE};

    VkImage msaaLightingColorImage{VK_NULL_HANDLE};
    VmaAllocation msaaLightingColorAllocation{nullptr};
    VkImageView msaaLightingColorImageView{VK_NULL_HANDLE};

    VkImage lightingColorImage{VK_NULL_HANDLE};
    VmaAllocation lightingColorAllocation{nullptr};
    VkImageView lightingColorImageView{VK_NULL_HANDLE};

    VkImage bloomExtractImage{VK_NULL_HANDLE};
    VmaAllocation bloomExtractAllocation{nullptr};
    VkImageView bloomExtractImageView{VK_NULL_HANDLE};

    VkImage bloomBlurImage{VK_NULL_HANDLE};
    VmaAllocation bloomBlurAllocation{nullptr};
    VkImageView bloomBlurImageView{VK_NULL_HANDLE};

    VkFramebuffer framebuffer{VK_NULL_HANDLE};
    VkFramebuffer lightingFramebuffer{VK_NULL_HANDLE};
    VkFramebuffer bloomExtractFramebuffer{VK_NULL_HANDLE};
    VkFramebuffer bloomBlurFramebuffer{VK_NULL_HANDLE};
};

struct RendererContext {
    Window* window{nullptr};

    Scope<VulkanContext> vulkanContext;
    Scope<Surface> surfaceWrapper;
    Scope<Device> deviceWrapper;
    Scope<SwapChain> swapChainWrapper;
    Scope<Pipeline> geometryPipeline;
    Scope<Pipeline> lightingPipeline;
    Scope<Pipeline> bloomExtractPipeline;
    Scope<Pipeline> bloomBlurPipeline;
    Scope<Pipeline> bloomVerticalPipeline;
    Scope<Pipeline> presentPipeline;
    Scope<Pipeline> billboardPipeline;

    Ref<Mesh> quadMesh;
    Ref<Mesh> cubeMesh;
    Ref<Mesh> sphereMesh;
    Ref<EditorCamera> camera;
    Ref<Scene> scene;

    Scope<DescriptorSetLayout> materialSetLayout;
    Scope<DescriptorPool> materialDescriptorPool;
    Scope<DescriptorSetLayout> animationSetLayout;
    Scope<DescriptorPool> animationDescriptorPool;
    Scope<DescriptorSetLayout> globalSetLayout;
    Scope<DescriptorPool> globalDescriptorPool;
    Scope<DescriptorSetLayout> compositeSetLayout;
    Scope<DescriptorPool> compositeDescriptorPool;
    Scope<DescriptorSetLayout> presentSetLayout;
    Scope<DescriptorPool> presentDescriptorPool;
    Scope<DescriptorSetLayout> bloomSetLayout;
    Scope<DescriptorPool> bloomDescriptorPool;
    Scope<DescriptorSetLayout> billboardSetLayout;
    Scope<DescriptorPool> billboardDescriptorPool;
    VkDescriptorSet pointLightIconDescriptorSet{VK_NULL_HANDLE};
    Ref<Texture> pointLightIconTexture;
    std::unordered_map<std::string, Ref<Texture>> textureCache;
    std::unordered_map<uint32_t, VkDescriptorSet> objectMaterialDescriptors;
    std::unordered_map<uint32_t, VkDescriptorSet> objectAnimationDescriptors;
    std::unordered_map<uint32_t, Scope<Buffer>> objectAnimationBuffers;
    std::unordered_map<uint32_t, std::string> objectBoundMaterialSignature;
    std::unordered_map<uint32_t, uint32_t> objectMaterialFlags;
    std::string boundEnvironmentSignature;
    std::vector<Scope<Buffer>> globalUboBuffers;
    std::vector<VkDescriptorSet> globalDescriptorSets;
    std::vector<VkDescriptorSet> compositeDescriptorSets;
    std::vector<VkDescriptorSet> presentDescriptorSets;
    std::vector<VkDescriptorSet> bloomExtractDescriptorSets;
    std::vector<VkDescriptorSet> bloomBlurDescriptorSets;
    std::vector<VkDescriptorSet> bloomVerticalDescriptorSets;

    Scope<ShaderLibrary> shaderLibrary;

    VkSurfaceKHR surface{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    VkQueue graphicsQueue{VK_NULL_HANDLE};
    VkQueue presentQueue{VK_NULL_HANDLE};

    VkFormat swapChainImageFormat{VK_FORMAT_UNDEFINED};
    VkExtent2D swapChainExtent{};
    VkSampleCountFlagBits msaaSamples{VK_SAMPLE_COUNT_1_BIT};
    VkRenderPass presentRenderPass{VK_NULL_HANDLE};
    VkRenderPass lightingRenderPass{VK_NULL_HANDLE};
    VkRenderPass bloomRenderPass{VK_NULL_HANDLE};
    VkRenderPass geometryRenderPass{VK_NULL_HANDLE};
    VkSampler compositeSampler{VK_NULL_HANDLE};
    VkFormat offscreenWorldPosRoughnessFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    VkFormat offscreenAlbedoAoFormat{VK_FORMAT_R8G8B8A8_UNORM};
    VkFormat offscreenLightingColorFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    VkFormat bloomParamsFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    VkFormat entityIdFormat{VK_FORMAT_R32G32_UINT};
    std::vector<OffscreenFrameResources> offscreenFrames;

    VkCommandPool commandPool{VK_NULL_HANDLE};
    VkPipelineLayout geometryPipelineLayout{VK_NULL_HANDLE};
    VkPipelineLayout lightingPipelineLayout{VK_NULL_HANDLE};
    VkPipelineLayout presentPipelineLayout{VK_NULL_HANDLE};
    VkPipelineLayout bloomPipelineLayout{VK_NULL_HANDLE};
    VkPipelineLayout billboardPipelineLayout{VK_NULL_HANDLE};

    std::vector<FrameResources> frameResources;
    std::vector<VkFence> imagesInFlight;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    size_t currentFrame{0};
    uint32_t lastRenderedImageIndex{0};
    bool hasRenderedFrame{false};

};

} // namespace Piece
