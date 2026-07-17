#include <PiecePCH.h>

#include <renderer/Renderer.h>
#include <renderer/Device.h>
#include <renderer/FrameInfo.h>
#include <renderer/FrameResources.h>
#include <renderer/RendererContext.h>
#include <renderer/Surface.h>
#include <renderer/SwapChain.h>
#include <renderer/Pipeline.h>
#include <renderer/Descriptors.h>
#include <renderer/Shader.h>
#include <renderer/ShaderLibrary.h>
#include <renderer/Texture.h>
#include <renderer/VulkanContext.h>
#include <renderer/RenderPass.h>
#include <renderer/RendererInternals.h>
#include <renderer/systems/LightingRenderSystem.h>
#include <renderer/systems/SceneRenderSystem.h>
#include <renderer/passes/GeometryPass.h>
#include <renderer/passes/LightingPass.h>
#include <scene/Components.h>
#include <scene/EditorCamera.h>
#include <scene/Entity.h>
#include <scene/Mesh.h>
#include <scene/PrimitiveMeshData.h>
#include <scene/Scene.h>
#include <scene/World.h>
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#ifndef PIECE_SHADER_DIR
#define PIECE_SHADER_DIR "./PieceLib/src/shaders"
#endif

namespace Piece
{

    namespace
    {
        std::unique_ptr<RendererContext> s_Context = nullptr;
        std::function<void()> s_SwapChainRecreatedCallback = nullptr;

        VkExtent2D GetValidSwapChainExtent(Window *window)
        {
            PIECE_CORE_ASSERT(window != nullptr, "Window must not be null");

            uint32_t width = window->GetWidth();
            uint32_t height = window->GetHeight();

            while (width == 0 || height == 0)
            {
                glfwWaitEvents();
                width = window->GetWidth();
                height = window->GetHeight();
            }

            return VkExtent2D{width, height};
        }

        std::string MakeMaterialSignature(const MaterialTextures &material)
        {
            return material.albedoPath + "|" + material.roughnessPath + "|" + material.ambientOcclusionPath;
        }

        std::shared_ptr<Texture> GetOrCreateTexture(RendererContext &ctx, const std::string &texturePath)
        {
            const std::string key = texturePath.empty() ? "__DEFAULT_WHITE__" : texturePath;
            auto it = ctx.textureCache.find(key);
            if (it != ctx.textureCache.end())
            {
                return it->second;
            }

            std::shared_ptr<Texture> texture = std::make_shared<Texture>(*ctx.deviceWrapper, texturePath);
            ctx.textureCache[key] = texture;
            return texture;
        }

        void EnsureObjectMaterialDescriptor(RendererContext &ctx, uint32_t objectId, uint32_t materialId, const MaterialTextures &material)
        {
            const MaterialTextures resolvedMaterial = World::ResolveMaterialTextures(materialId, material);
            const std::string materialSignature = MakeMaterialSignature(resolvedMaterial);
            const bool hasDescriptor = ctx.objectMaterialDescriptors.find(objectId) != ctx.objectMaterialDescriptors.end();
            const bool pathChanged = ctx.objectBoundMaterialSignature[objectId] != materialSignature;

            if (hasDescriptor && !pathChanged)
            {
                return;
            }

            if (!hasDescriptor)
            {
                VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
                const bool allocated = ctx.materialDescriptorPool->allocateDescriptor(ctx.materialSetLayout->getDescriptorSetLayout(), descriptorSet);
                PIECE_CORE_ASSERT(allocated, "Failed to allocate material descriptor set");
                ctx.objectMaterialDescriptors[objectId] = descriptorSet;
            }

            auto albedoTexture = GetOrCreateTexture(ctx, resolvedMaterial.albedoPath);
            auto roughnessTexture = GetOrCreateTexture(ctx, resolvedMaterial.roughnessPath);
            auto aoTexture = GetOrCreateTexture(ctx, resolvedMaterial.ambientOcclusionPath);

            VkDescriptorImageInfo albedoImageInfo{};
            albedoImageInfo.sampler = albedoTexture->getSampler();
            albedoImageInfo.imageView = albedoTexture->getImageView();
            albedoImageInfo.imageLayout = albedoTexture->getImageLayout();

            VkDescriptorImageInfo roughnessImageInfo{};
            roughnessImageInfo.sampler = roughnessTexture->getSampler();
            roughnessImageInfo.imageView = roughnessTexture->getImageView();
            roughnessImageInfo.imageLayout = roughnessTexture->getImageLayout();

            VkDescriptorImageInfo aoImageInfo{};
            aoImageInfo.sampler = aoTexture->getSampler();
            aoImageInfo.imageView = aoTexture->getImageView();
            aoImageInfo.imageLayout = aoTexture->getImageLayout();

            DescriptorWriter writer(*ctx.materialSetLayout, *ctx.materialDescriptorPool);
            writer.writeImage(0, &albedoImageInfo);
            writer.writeImage(1, &roughnessImageInfo);
            writer.writeImage(2, &aoImageInfo);
            writer.overwrite(ctx.objectMaterialDescriptors[objectId]);
            ctx.objectBoundMaterialSignature[objectId] = materialSignature;
        }

        uint32_t SpawnPrimitive(
            PrimitiveType primitiveType,
            const glm::vec3 &position,
            const glm::vec3 &rotation,
            const glm::vec3 &scale)
        {
            if (!s_Context || !s_Context->scene)
            {
                return 0;
            }

            RendererContext &ctx = *s_Context;
            Entity entity;
            switch (primitiveType)
            {
            case PrimitiveType::Cube:
                entity = ctx.scene->CreateCube();
                break;
            case PrimitiveType::Sphere:
                entity = ctx.scene->CreateSphere();
                break;
            case PrimitiveType::Quad:
            default:
                entity = ctx.scene->CreateQuad();
                break;
            }
            auto &transform = entity.GetComponent<TransformComponent>();
            transform.position = position;
            transform.rotation = rotation;
            transform.scale = scale;
            return static_cast<uint32_t>(entity);
        }

        PointLightSettings BuildDefaultPointLight(const glm::vec3 &basePosition, uint32_t index)
        {
            PointLightSettings light{};
            light.position = basePosition + glm::vec3(
                                                (index % 2 == 0) ? 0.8f : -0.8f,
                                                0.8f + 0.25f * static_cast<float>(index),
                                                0.4f);
            light.radius = 6.0f;
            light.intensity = 2.4f;

            if (index % 2 == 0)
            {
                light.color = glm::vec3(1.0f, 0.85f, 0.75f);
            }
            else
            {
                light.color = glm::vec3(0.7f, 0.85f, 1.0f);
            }

            return light;
        }

        FrameInfo BuildFrameInfo(
            const FrameResources &frameResources,
            VkRenderPass renderPass,
            VkFramebuffer framebuffer,
            VkPipelineLayout pipelineLayout,
            Pipeline *pipeline,
            bool bindGlobalDescriptors)
        {
            RendererContext &ctx = *s_Context;

            FrameInfo frameInfo{};
            frameInfo.commandBuffer = frameResources.commandBuffer;
            frameInfo.imageIndex = frameResources.imageIndex;
            frameInfo.swapChainExtent = ctx.swapChainExtent;
            frameInfo.renderPass = renderPass;
            frameInfo.framebuffer = framebuffer;
            frameInfo.pipelineLayout = pipelineLayout;
            frameInfo.globalDescriptorSet = bindGlobalDescriptors && (ctx.currentFrame < ctx.globalDescriptorSets.size())
                                                ? ctx.globalDescriptorSets[ctx.currentFrame]
                                                : VK_NULL_HANDLE;
            frameInfo.materialDescriptorSets = &ctx.objectMaterialDescriptors;
            frameInfo.pipeline = pipeline;
            frameInfo.camera = ctx.camera.get();
            frameInfo.scene = ctx.scene.get();

            return frameInfo;
        }

        void RecordCommandBuffer(const FrameInfo &frameInfo)
        {
            VkCommandBuffer commandBuffer = frameInfo.commandBuffer;
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            const RendererContext &ctx = *s_Context;
            GeometryPass::Record(ctx, frameInfo);
            LightingPass::Record(ctx, frameInfo);

            vkEndCommandBuffer(commandBuffer);
        }

        VkSampleCountFlagBits ChooseMsaaSamples(const VkPhysicalDeviceProperties& props)
        {
            VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts &
            props.limits.framebufferDepthSampleCounts;
            
            PIECE_CORE_INFO("Available MSAA sample counts mask: {}", static_cast<uint32_t>(counts));
            if (counts & VK_SAMPLE_COUNT_8_BIT)
                return VK_SAMPLE_COUNT_8_BIT;

            if (counts & VK_SAMPLE_COUNT_4_BIT)
                return VK_SAMPLE_COUNT_4_BIT;

            if (counts & VK_SAMPLE_COUNT_2_BIT)
                return VK_SAMPLE_COUNT_2_BIT;

            return VK_SAMPLE_COUNT_1_BIT;
        }
    }

    void Renderer::Init(Window *window)
    {
        PIECE_CORE_ASSERT(window != nullptr, "Window must not be null");
        s_Context = std::make_unique<RendererContext>();
        RendererContext &ctx = *s_Context;

        ctx.window = window;
        ctx.vulkanContext = std::make_unique<VulkanContext>();
        ctx.surfaceWrapper = std::make_unique<Surface>(*ctx.vulkanContext, window);
        ctx.deviceWrapper = std::make_unique<Device>(*ctx.surfaceWrapper, *ctx.vulkanContext);

        ctx.device = ctx.deviceWrapper->device();
        ctx.physicalDevice = ctx.deviceWrapper->getPhysicalDevice();
        ctx.surface = ctx.surfaceWrapper->surface();
        ctx.graphicsQueue = ctx.deviceWrapper->graphicsQueue();
        ctx.presentQueue = ctx.deviceWrapper->presentQueue();
        ctx.scene = World::GetActiveScene();

        ctx.msaaSamples = ChooseMsaaSamples(ctx.deviceWrapper->properties);

        // create swapchain wrapper which also creates image views, render pass, framebuffers and sync
        VkExtent2D extent = GetValidSwapChainExtent(ctx.window);
        ctx.swapChainWrapper = std::make_unique<SwapChain>(*ctx.deviceWrapper, extent);

        ctx.swapChainImageFormat = ctx.swapChainWrapper->getSwapChainImageFormat();
        ctx.swapChainExtent = ctx.swapChainWrapper->getSwapChainExtent();
        ctx.lightingRenderPass = ctx.swapChainWrapper->getRenderPass();
        ctx.offscreenWorldPosRoughnessFormat = ctx.deviceWrapper->findSupportedFormat(
            {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        ctx.offscreenAlbedoAoFormat = ctx.deviceWrapper->findSupportedFormat(
            {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(ScenePushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        ctx.materialSetLayout = DescriptorSetLayout::Builder(*ctx.deviceWrapper)
                                    .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                    .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                    .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                    .build();

        LightingRenderSystem::Initialize(ctx);

        ctx.materialDescriptorPool = DescriptorPool::Builder(*ctx.deviceWrapper)
                                         .setMaxSets(2048)
                                         .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2048 * 3)
                                         .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                                         .build();

        RendererInternals::CreateGeometryRenderPass(ctx);
        RendererInternals::CreateOffscreenResources(ctx);
        RendererInternals::CreateCompositeResources(ctx);

        std::vector<VkDescriptorSetLayout> geometrySetLayouts{
            ctx.materialSetLayout->getDescriptorSetLayout(),
            ctx.globalSetLayout->getDescriptorSetLayout()};
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(geometrySetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = geometrySetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        VkResult result = vkCreatePipelineLayout(ctx.device, &pipelineLayoutInfo, nullptr, &ctx.geometryPipelineLayout);
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create geometry pipeline layout");

        std::vector<VkDescriptorSetLayout> lightingSetLayouts{
            ctx.compositeSetLayout->getDescriptorSetLayout(),
            ctx.globalSetLayout->getDescriptorSetLayout()};
        VkPipelineLayoutCreateInfo lightingLayoutInfo{};
        lightingLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        lightingLayoutInfo.setLayoutCount = static_cast<uint32_t>(lightingSetLayouts.size());
        lightingLayoutInfo.pSetLayouts = lightingSetLayouts.data();
        result = vkCreatePipelineLayout(ctx.device, &lightingLayoutInfo, nullptr, &ctx.lightingPipelineLayout);
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create lighting pipeline layout");

        const PrimitiveMeshData quadMeshData = PrimitiveMeshDataFactory::CreateQuad();
        ctx.quadMesh = std::make_shared<Mesh>(*ctx.deviceWrapper, quadMeshData.vertices, quadMeshData.indices);

        const PrimitiveMeshData cubeMeshData = PrimitiveMeshDataFactory::CreateCube();
        ctx.cubeMesh = std::make_shared<Mesh>(*ctx.deviceWrapper, cubeMeshData.vertices, cubeMeshData.indices);

        const PrimitiveMeshData sphereMeshData = PrimitiveMeshDataFactory::CreateSphere();
        ctx.sphereMesh = std::make_shared<Mesh>(*ctx.deviceWrapper, sphereMeshData.vertices, sphereMeshData.indices);

        ctx.camera = std::make_shared<EditorCamera>(70.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 100.0f);
        if (ctx.scene)
        {
            ctx.scene->OnViewportResize(extent.width, extent.height);
        }

        // Load shaders through the library — loaded once, reused across pipeline recreations.
        ctx.shaderLibrary = std::make_unique<ShaderLibrary>(*ctx.deviceWrapper);
        ctx.shaderLibrary->Load("textured.vert", PIECE_SHADER_DIR "/textured.vert.spv", Shader::Stage::Vertex);
        ctx.shaderLibrary->Load("textured.frag", PIECE_SHADER_DIR "/textured.frag.spv", Shader::Stage::Fragment);
        ctx.shaderLibrary->Load("lighting_composite.vert", PIECE_SHADER_DIR "/lighting_composite.vert.spv", Shader::Stage::Vertex);
        ctx.shaderLibrary->Load("lighting_composite.frag", PIECE_SHADER_DIR "/lighting_composite.frag.spv", Shader::Stage::Fragment);

        RendererInternals::CreateGraphicsPipeline(ctx);
        // command pool is created by Device; get it
        ctx.commandPool = ctx.deviceWrapper->getCommandPool();
        CreateCommandBuffers();
        ctx.imagesInFlight.assign(ctx.swapChainWrapper->imageCount(), VK_NULL_HANDLE);

        PIECE_CORE_INFO("{}x MSAA enabled", static_cast<uint32_t>(ctx.msaaSamples));
    }

    void Renderer::Shutdown()
    {
        if (!s_Context || s_Context->device == VK_NULL_HANDLE)
        {
            return;
        }

        RendererContext &ctx = *s_Context;

        vkDeviceWaitIdle(ctx.device);

        ctx.quadMesh.reset();
        ctx.cubeMesh.reset();
        ctx.sphereMesh.reset();
        ctx.camera.reset();
        ctx.objectMaterialDescriptors.clear();
        ctx.objectBoundMaterialSignature.clear();
        ctx.textureCache.clear();
        LightingRenderSystem::Shutdown(ctx);
        RendererInternals::DestroyCompositeResources(ctx);
        RendererInternals::DestroyOffscreenResources(ctx);
        RendererInternals::DestroyGeometryRenderPass(ctx);
        ctx.materialDescriptorPool.reset();
        ctx.materialSetLayout.reset();
        ctx.shaderLibrary.reset();

        ctx.geometryPipeline.reset();
        ctx.lightingPipeline.reset();
        RendererInternals::DestroyPipelineLayouts(ctx);

        CleanupSwapChain();

        for (FrameResources &frame : ctx.frameResources)
        {
            if (frame.renderFinishedSemaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(ctx.device, frame.renderFinishedSemaphore, nullptr);
                frame.renderFinishedSemaphore = VK_NULL_HANDLE;
            }
            if (frame.imageAvailableSemaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(ctx.device, frame.imageAvailableSemaphore, nullptr);
                frame.imageAvailableSemaphore = VK_NULL_HANDLE;
            }
            if (frame.inFlightFence != VK_NULL_HANDLE)
            {
                vkDestroyFence(ctx.device, frame.inFlightFence, nullptr);
                frame.inFlightFence = VK_NULL_HANDLE;
            }
        }
        ctx.frameResources.clear();

        ctx.swapChainWrapper.reset();
        ctx.deviceWrapper.reset();
        ctx.surfaceWrapper.reset();
        ctx.vulkanContext.reset();
        s_Context.reset();
    }

    void Renderer::WaitIdle()
    {
        if (s_Context && s_Context->device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(s_Context->device);
        }
    }

    void Renderer::DrawFrame()
    {
        PIECE_CORE_ASSERT(s_Context != nullptr, "Renderer context is not initialized");
        RendererContext &ctx = *s_Context;

        uint32_t imageIndex;
        FrameResources &frameResources = ctx.frameResources[ctx.currentFrame];
        VkResult result = ctx.swapChainWrapper->acquireNextImage(&imageIndex, frameResources);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapChain();
            return;
        }
        PIECE_CORE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to acquire swapchain image");

        frameResources.imageIndex = imageIndex;

        LightingRenderSystem::UpdatePerFrame(ctx, static_cast<uint32_t>(ctx.currentFrame));

        // Update material descriptors outside command buffer recording.
        // vkDeviceWaitIdle is only called when at least one object needs an update.
        {
            bool anyPending = false;
            auto view = ctx.scene->GetAllEntitiesViewWith<MeshRendererComponent>();
            for (auto entityHandle : view)
            {
                const auto &meshRenderer = view.get<MeshRendererComponent>(entityHandle);
                uint32_t id = static_cast<uint32_t>(entityHandle);
                bool noDescriptor = ctx.objectMaterialDescriptors.find(id) == ctx.objectMaterialDescriptors.end();
                MaterialTextures resolvedMaterial = World::ResolveMaterialTextures(meshRenderer.materialId, meshRenderer.materialTextures);
                bool pathChanged = ctx.objectBoundMaterialSignature[id] != MakeMaterialSignature(resolvedMaterial);
                if (noDescriptor || pathChanged)
                {
                    anyPending = true;
                    break;
                }
            }
            if (anyPending)
            {
                vkDeviceWaitIdle(ctx.device);
                for (auto entityHandle : view)
                {
                    const auto &meshRenderer = view.get<MeshRendererComponent>(entityHandle);
                    EnsureObjectMaterialDescriptor(ctx, static_cast<uint32_t>(entityHandle), meshRenderer.materialId, meshRenderer.materialTextures);
                }
            }
        }

        vkResetCommandBuffer(frameResources.commandBuffer, 0);
        FrameInfo frameInfo = BuildFrameInfo(
            frameResources,
            ctx.geometryRenderPass,
            ctx.offscreenFrames[frameResources.imageIndex].framebuffer,
            ctx.geometryPipelineLayout,
            ctx.geometryPipeline.get(),
            true);
        RecordCommandBuffer(frameInfo);

        result = ctx.swapChainWrapper->submitCommandBuffers(&frameResources.commandBuffer, &imageIndex, frameResources, ctx.imagesInFlight);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            RecreateSwapChain();
        }
        PIECE_CORE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to submit swapchain command buffers");

        ctx.currentFrame = (ctx.currentFrame + 1) % ctx.frameResources.size();
    }

    void Renderer::Update(Timestep ts)
    {
        (void)ts;

        if (!s_Context)
        {
            return;
        }

        RendererContext &ctx = *s_Context;

        if (ctx.camera)
        {
            if (ImGui::GetCurrentContext() != nullptr)
            {
                ImGuiIO &io = ImGui::GetIO();
                if (io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput)
                {
                    return;
                }
            }
            ctx.camera->onUpdate(static_cast<float>(ts));
        }
    }

    void Renderer::OnWindowResize(uint32_t width, uint32_t height)
    {
        if (!s_Context)
        {
            return;
        }

        if (width == 0 || height == 0)
        {
            return;
        }

        if (s_Context->camera)
        {
            s_Context->camera->setViewportSize(static_cast<float>(width), static_cast<float>(height));
        }
        if (s_Context->scene)
        {
            s_Context->scene->OnViewportResize(width, height);
        }

        RecreateSwapChain();
    }

    bool Renderer::OnMouseScrolled(MouseScrolledEvent &event)
    {
        if (ImGui::GetCurrentContext() != nullptr)
        {
            ImGuiIO &io = ImGui::GetIO();
            if (io.WantCaptureMouse)
            {
                return true;
            }
        }

        if (s_Context && s_Context->camera)
        {
            s_Context->camera->onMouseScroll(event.GetOffsetY());
        }
        return false;
    }

    Device &Renderer::GetDevice()
    {
        PIECE_CORE_ASSERT(s_Context && s_Context->deviceWrapper, "Renderer device is not initialized");
        return *s_Context->deviceWrapper;
    }

    RenderPass &Renderer::GetRenderPass()
    {
        PIECE_CORE_ASSERT(s_Context && s_Context->swapChainWrapper, "Renderer swapchain is not initialized");
        return s_Context->swapChainWrapper->getRenderPassObject();
    }

    VulkanContext &Renderer::GetVulkanContext()
    {
        PIECE_CORE_ASSERT(s_Context && s_Context->vulkanContext, "Renderer Vulkan context is not initialized");
        return *s_Context->vulkanContext;
    }

    void Renderer::SetSwapChainRecreatedCallback(const std::function<void()> &callback)
    {
        s_SwapChainRecreatedCallback = callback;
    }

    bool Renderer::CreatePointLightInView()
    {
        if (!s_Context || !s_Context->scene)
        {
            return false;
        }

        const glm::vec3 spawnCenter = s_Context->camera
                                          ? s_Context->camera->focalPoint()
                                          : glm::vec3(0.0f);
        Entity pointLight = s_Context->scene->CreatePointLight();
        PointLightSettings defaults = BuildDefaultPointLight(spawnCenter, 0);
        auto &transform = pointLight.GetComponent<TransformComponent>();
        auto &light = pointLight.GetComponent<PointLightComponent>();
        transform.position = defaults.position;
        light.radius = defaults.radius;
        light.color = defaults.color;
        light.intensity = defaults.intensity;
        return true;
    }

    void Renderer::CreateQuadInView()
    {
        if (!s_Context || !s_Context->camera)
        {
            return;
        }

        SpawnPrimitive(PrimitiveType::Quad, s_Context->camera->focalPoint(), glm::vec3(0.0f), glm::vec3(1.0f));
    }

    void Renderer::CreateCubeInView()
    {
        if (!s_Context || !s_Context->camera)
        {
            return;
        }

        SpawnPrimitive(PrimitiveType::Cube, s_Context->camera->focalPoint(), glm::vec3(0.0f), glm::vec3(1.0f));
    }

    uint32_t Renderer::CreateQuad(const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale)
    {
        if (!s_Context)
        {
            return 0;
        }

        return SpawnPrimitive(PrimitiveType::Quad, position, rotation, scale);
    }

    uint32_t Renderer::CreateCube(const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale)
    {
        if (!s_Context)
        {
            return 0;
        }

        return SpawnPrimitive(PrimitiveType::Cube, position, rotation, scale);
    }

    uint32_t Renderer::CreateSphere(const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale)
    {
        if (!s_Context)
        {
            return 0;
        }

        return SpawnPrimitive(PrimitiveType::Sphere, position, rotation, scale);
    }

    void Renderer::CreateSphereInView()
    {
        if (!s_Context || !s_Context->camera)
        {
            return;
        }

        SpawnPrimitive(PrimitiveType::Sphere, s_Context->camera->focalPoint(), glm::vec3(0.0f), glm::vec3(1.0f));
    }

    /* Command pool is created by Device; Renderer uses Device::getCommandPool() */

    void Renderer::CreateCommandBuffers()
    {
        RendererContext &ctx = *s_Context;
        size_t count = SwapChain::MAX_FRAMES_IN_FLIGHT;

        if (ctx.frameResources.empty())
        {
            ctx.frameResources.resize(count);
        }

        std::vector<VkCommandBuffer> commandBuffers(count);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = ctx.commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        VkResult result = vkAllocateCommandBuffers(ctx.device, &allocInfo, commandBuffers.data());
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffers");

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < commandBuffers.size(); i++)
        {
            ctx.frameResources[i].commandBuffer = commandBuffers[i];
            ctx.frameResources[i].imageIndex = static_cast<uint32_t>(i);

            if (ctx.frameResources[i].imageAvailableSemaphore == VK_NULL_HANDLE)
            {
                result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.frameResources[i].imageAvailableSemaphore);
                PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create image-available semaphore");
            }
            if (ctx.frameResources[i].renderFinishedSemaphore == VK_NULL_HANDLE)
            {
                result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.frameResources[i].renderFinishedSemaphore);
                PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create render-finished semaphore");
            }
            if (ctx.frameResources[i].inFlightFence == VK_NULL_HANDLE)
            {
                result = vkCreateFence(ctx.device, &fenceInfo, nullptr, &ctx.frameResources[i].inFlightFence);
                PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create in-flight fence");
            }

            FrameInfo frameInfo = BuildFrameInfo(
                ctx.frameResources[i],
                ctx.geometryRenderPass,
                ctx.offscreenFrames[ctx.frameResources[i].imageIndex].framebuffer,
                ctx.geometryPipelineLayout,
                ctx.geometryPipeline.get(),
                true);
            RecordCommandBuffer(frameInfo);
        }
    }

    void Renderer::CleanupSwapChain()
    {
        if (!s_Context)
        {
            return;
        }

        RendererContext &ctx = *s_Context;

        // Command buffers belong to renderer
        if (!ctx.frameResources.empty())
        {
            std::vector<VkCommandBuffer> commandBuffers;
            commandBuffers.reserve(ctx.frameResources.size());

            for (const FrameResources &frame : ctx.frameResources)
            {
                commandBuffers.push_back(frame.commandBuffer);
            }

            vkFreeCommandBuffers(ctx.device, ctx.commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

            for (FrameResources &frame : ctx.frameResources)
            {
                frame.commandBuffer = VK_NULL_HANDLE;
            }
        }
    }

    void Renderer::RecreateSwapChain()
    {
        RendererContext &ctx = *s_Context;

        vkDeviceWaitIdle(ctx.device);

        CleanupSwapChain();

        ctx.geometryPipeline.reset();
        ctx.lightingPipeline.reset();
        RendererInternals::DestroyPipelineLayouts(ctx);
        RendererInternals::DestroyCompositeResources(ctx);
        RendererInternals::DestroyOffscreenResources(ctx);
        RendererInternals::DestroyGeometryRenderPass(ctx);

        ctx.swapChainWrapper.reset();

        VkExtent2D extent = GetValidSwapChainExtent(ctx.window);
        ctx.swapChainWrapper = std::make_unique<SwapChain>(*ctx.deviceWrapper, extent);

        ctx.swapChainImageFormat = ctx.swapChainWrapper->getSwapChainImageFormat();
        ctx.swapChainExtent = ctx.swapChainWrapper->getSwapChainExtent();
        ctx.lightingRenderPass = ctx.swapChainWrapper->getRenderPass();
        ctx.offscreenWorldPosRoughnessFormat = ctx.deviceWrapper->findSupportedFormat(
            {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        ctx.offscreenAlbedoAoFormat = ctx.deviceWrapper->findSupportedFormat(
            {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        ctx.imagesInFlight.assign(ctx.swapChainWrapper->imageCount(), VK_NULL_HANDLE);
        ctx.objectBoundMaterialSignature.clear();
        if (ctx.scene)
        {
            ctx.scene->OnViewportResize(extent.width, extent.height);
        }

        ctx.msaaSamples = ChooseMsaaSamples(ctx.deviceWrapper->properties);

        RendererInternals::CreateGeometryRenderPass(ctx);
        RendererInternals::CreateOffscreenResources(ctx);
        RendererInternals::CreateCompositeResources(ctx);

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(ScenePushConstants);

        std::vector<VkDescriptorSetLayout> geometrySetLayouts{
            ctx.materialSetLayout->getDescriptorSetLayout(),
            ctx.globalSetLayout->getDescriptorSetLayout()};
        VkPipelineLayoutCreateInfo geometryLayoutInfo{};
        geometryLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        geometryLayoutInfo.setLayoutCount = static_cast<uint32_t>(geometrySetLayouts.size());
        geometryLayoutInfo.pSetLayouts = geometrySetLayouts.data();
        geometryLayoutInfo.pushConstantRangeCount = 1;
        geometryLayoutInfo.pPushConstantRanges = &pushConstantRange;
        VkResult result = vkCreatePipelineLayout(ctx.device, &geometryLayoutInfo, nullptr, &ctx.geometryPipelineLayout);
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to recreate geometry pipeline layout");

        std::vector<VkDescriptorSetLayout> lightingSetLayouts{
            ctx.compositeSetLayout->getDescriptorSetLayout(),
            ctx.globalSetLayout->getDescriptorSetLayout()};
        VkPipelineLayoutCreateInfo lightingLayoutInfo{};
        lightingLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        lightingLayoutInfo.setLayoutCount = static_cast<uint32_t>(lightingSetLayouts.size());
        lightingLayoutInfo.pSetLayouts = lightingSetLayouts.data();
        result = vkCreatePipelineLayout(ctx.device, &lightingLayoutInfo, nullptr, &ctx.lightingPipelineLayout);
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to recreate lighting pipeline layout");

        RendererInternals::CreateGraphicsPipeline(ctx);
        CreateCommandBuffers();

        if (s_SwapChainRecreatedCallback)
        {
            s_SwapChainRecreatedCallback();
        }
    }

} // namespace Piece
