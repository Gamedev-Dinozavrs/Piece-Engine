#include <PiecePCH.h>
#include <Piece.h>
#include <renderer/Buffer.h>

#include "imgui.h"
#include <GLFW/glfw3.h>

#ifndef PIECE_SHADER_DIR
#define PIECE_SHADER_DIR "./PieceLib/src/shaders"
#endif

namespace Piece
{

    namespace
    {
        Scope<RendererContext> s_Context = nullptr;
        std::function<void()> s_SwapChainRecreatedCallback = nullptr;

        constexpr uint32_t kMaterialFlagHasNormalMap = 1u << 0;
        constexpr uint32_t kMaterialFlagHasEmissiveMap = 1u << 1;

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
            return material.albedoPath + "|" + material.normalPath + "|" + material.roughnessPath + "|" + material.metallicPath + "|" + material.ambientOcclusionPath + "|" + material.emissivePath;
        }

        std::string MakeEnvironmentSignature(const EnvironmentSettings& environment)
        {
            return (environment.enabled ? "1|" : "0|")
                + environment.diffuseMapPath + "|"
                + environment.specularMapPath + "|"
                + std::to_string(environment.intensity) + "|"
                + std::to_string(environment.diffuseStrength) + "|"
                + std::to_string(environment.specularStrength) + "|"
                + std::to_string(static_cast<int>(environment.aaTechnique)) + "|"
                + std::to_string(environment.msaaSampleCount);
        }

        Ref<Texture> GetOrCreateTexture(RendererContext &ctx, const std::string &texturePath, bool isColorData = true)
        {
            const std::string key = (texturePath.empty() ? "__DEFAULT_WHITE__" : texturePath)
                + (isColorData ? "|color" : "|data");
            auto it = ctx.textureCache.find(key);
            if (it != ctx.textureCache.end())
            {
                return it->second;
            }

            Ref<Texture> texture = CreateRef<Texture>(*ctx.deviceWrapper, texturePath, isColorData);
            ctx.textureCache[key] = texture;
            return texture;
        }

        void EnsureCompositeEnvironmentDescriptors(RendererContext& ctx)
        {
            if (!ctx.compositeSetLayout || !ctx.compositeDescriptorPool || ctx.compositeDescriptorSets.empty()) {
                return;
            }

            const EnvironmentSettings environment = World::GetEnvironmentSettings();
            const std::string signature = MakeEnvironmentSignature(environment);
            if (ctx.boundEnvironmentSignature == signature) {
                return;
            }

            const bool useMsaaDescriptors =
                environment.aaTechnique == AATechnique::MSAA &&
                ctx.msaaSamples != VK_SAMPLE_COUNT_1_BIT;

            auto envDiffuseTexture = GetOrCreateTexture(ctx, environment.diffuseMapPath);
            auto envSpecularTexture = GetOrCreateTexture(ctx, environment.specularMapPath);

            for (size_t i = 0; i < ctx.compositeDescriptorSets.size(); ++i) {
                VkDescriptorImageInfo worldPosRoughnessInfo{};
                worldPosRoughnessInfo.sampler = ctx.compositeSampler;
                worldPosRoughnessInfo.imageView = useMsaaDescriptors
                    ? ctx.offscreenFrames[i].msaaWorldPosRoughnessImageView
                    : ctx.offscreenFrames[i].worldPosRoughnessImageView;
                worldPosRoughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkDescriptorImageInfo albedoAoInfo{};
                albedoAoInfo.sampler = ctx.compositeSampler;
                albedoAoInfo.imageView = useMsaaDescriptors
                    ? ctx.offscreenFrames[i].msaaAlbedoAoImageView
                    : ctx.offscreenFrames[i].albedoAoImageView;
                albedoAoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkDescriptorImageInfo normalAoInfo{};
                normalAoInfo.sampler = ctx.compositeSampler;
                normalAoInfo.imageView = useMsaaDescriptors
                    ? ctx.offscreenFrames[i].msaaNormalAoImageView
                    : ctx.offscreenFrames[i].normalAoImageView;
                normalAoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkDescriptorImageInfo emissiveInfo{};
                emissiveInfo.sampler = ctx.compositeSampler;
                emissiveInfo.imageView = useMsaaDescriptors
                    ? ctx.offscreenFrames[i].msaaEmissiveImageView
                    : ctx.offscreenFrames[i].emissiveImageView;
                emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkDescriptorImageInfo envDiffuseInfo{};
                envDiffuseInfo.sampler = envDiffuseTexture->getSampler();
                envDiffuseInfo.imageView = envDiffuseTexture->getImageView();
                envDiffuseInfo.imageLayout = envDiffuseTexture->getImageLayout();

                VkDescriptorImageInfo envSpecularInfo{};
                envSpecularInfo.sampler = envSpecularTexture->getSampler();
                envSpecularInfo.imageView = envSpecularTexture->getImageView();
                envSpecularInfo.imageLayout = envSpecularTexture->getImageLayout();

                DescriptorWriter(*ctx.compositeSetLayout, *ctx.compositeDescriptorPool)
                    .writeImage(0, &worldPosRoughnessInfo)
                    .writeImage(1, &albedoAoInfo)
                    .writeImage(2, &normalAoInfo)
                    .writeImage(3, &emissiveInfo)
                    .writeImage(4, &envDiffuseInfo)
                    .writeImage(5, &envSpecularInfo)
                    .overwrite(ctx.compositeDescriptorSets[i]);
            }

            ctx.boundEnvironmentSignature = signature;
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
            auto normalTexture = GetOrCreateTexture(ctx, resolvedMaterial.normalPath, false);
            auto roughnessTexture = GetOrCreateTexture(ctx, resolvedMaterial.roughnessPath, false);
            auto metallicTexture = GetOrCreateTexture(ctx, resolvedMaterial.metallicPath, false);
            auto aoTexture = GetOrCreateTexture(ctx, resolvedMaterial.ambientOcclusionPath, false);
            auto emissiveTexture = GetOrCreateTexture(ctx, resolvedMaterial.emissivePath);

            VkDescriptorImageInfo albedoImageInfo{};
            albedoImageInfo.sampler = albedoTexture->getSampler();
            albedoImageInfo.imageView = albedoTexture->getImageView();
            albedoImageInfo.imageLayout = albedoTexture->getImageLayout();

            VkDescriptorImageInfo normalImageInfo{};
            normalImageInfo.sampler = normalTexture->getSampler();
            normalImageInfo.imageView = normalTexture->getImageView();
            normalImageInfo.imageLayout = normalTexture->getImageLayout();

            VkDescriptorImageInfo roughnessImageInfo{};
            roughnessImageInfo.sampler = roughnessTexture->getSampler();
            roughnessImageInfo.imageView = roughnessTexture->getImageView();
            roughnessImageInfo.imageLayout = roughnessTexture->getImageLayout();

            VkDescriptorImageInfo metallicImageInfo{};
            metallicImageInfo.sampler = metallicTexture->getSampler();
            metallicImageInfo.imageView = metallicTexture->getImageView();
            metallicImageInfo.imageLayout = metallicTexture->getImageLayout();

            VkDescriptorImageInfo aoImageInfo{};
            aoImageInfo.sampler = aoTexture->getSampler();
            aoImageInfo.imageView = aoTexture->getImageView();
            aoImageInfo.imageLayout = aoTexture->getImageLayout();

            VkDescriptorImageInfo emissiveImageInfo{};
            emissiveImageInfo.sampler = emissiveTexture->getSampler();
            emissiveImageInfo.imageView = emissiveTexture->getImageView();
            emissiveImageInfo.imageLayout = emissiveTexture->getImageLayout();

            DescriptorWriter writer(*ctx.materialSetLayout, *ctx.materialDescriptorPool);
            writer.writeImage(0, &albedoImageInfo);
            writer.writeImage(1, &normalImageInfo);
            writer.writeImage(2, &roughnessImageInfo);
            writer.writeImage(3, &metallicImageInfo);
            writer.writeImage(4, &aoImageInfo);
            writer.writeImage(5, &emissiveImageInfo);
            writer.overwrite(ctx.objectMaterialDescriptors[objectId]);

            uint32_t materialFlags = 0;
            if (!resolvedMaterial.normalPath.empty()) {
                materialFlags |= kMaterialFlagHasNormalMap;
            }
            if (!resolvedMaterial.metallicPath.empty()) {
                materialFlags |= 1u << 2;
            }
            if (!resolvedMaterial.emissivePath.empty()) {
                materialFlags |= kMaterialFlagHasEmissiveMap;
            }
            ctx.objectMaterialFlags[objectId] = materialFlags;
            ctx.objectBoundMaterialSignature[objectId] = materialSignature;
        }

        uint32_t SpawnPrimitive(
            PrimitiveType primitiveType,
            const glm::vec3 &position,
            const glm::vec3 &rotation,
            const glm::vec3 &scale)
        {
            if (!s_Context)
            {
                return 0;
            }

            SpawnTransform transform{};
            transform.position = position;
            transform.rotation = rotation;
            transform.scale = scale;
            return World::SpawnPrimitive(primitiveType, transform);
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

            RendererContext &ctx = *s_Context;
            GeometryPass::Record(ctx, frameInfo);
            LightingPass::Record(ctx, frameInfo);

            vkEndCommandBuffer(commandBuffer);
        }

        VkSampleCountFlagBits ResolveMsaaSamples(const VkPhysicalDeviceProperties& props, AATechnique technique, uint32_t requestedSamples)
        {
            VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts &
            props.limits.framebufferDepthSampleCounts;

            if (technique != AATechnique::MSAA) {
                return VK_SAMPLE_COUNT_1_BIT;
            }

            const VkSampleCountFlagBits preferredCounts[] = {
                VK_SAMPLE_COUNT_8_BIT,
                VK_SAMPLE_COUNT_4_BIT,
                VK_SAMPLE_COUNT_2_BIT,
                VK_SAMPLE_COUNT_1_BIT,
            };

            for (VkSampleCountFlagBits sampleCount : preferredCounts) {
                if (static_cast<uint32_t>(sampleCount) <= requestedSamples && (counts & sampleCount)) {
                    return sampleCount;
                }
            }

            return VK_SAMPLE_COUNT_1_BIT;
        }

        VkSampleCountFlagBits ChooseMsaaSamples(const VkPhysicalDeviceProperties& props, AATechnique technique, uint32_t requestedSamples)
        {
            VkSampleCountFlagBits sampleCount = ResolveMsaaSamples(props, technique, requestedSamples);
            PIECE_CORE_INFO(
                "Selected AA technique {} requested MSAA {} -> resolved MSAA {}",
                static_cast<int>(technique),
                requestedSamples,
                static_cast<uint32_t>(sampleCount));
            return sampleCount;
        }
    }

    void Renderer::Init(Window *window)
    {
        PIECE_CORE_ASSERT(window != nullptr, "Window must not be null");
        s_Context = CreateScope<RendererContext>();
        RendererContext &ctx = *s_Context;

        ctx.window = window;
        ctx.vulkanContext = CreateScope<VulkanContext>();
        ctx.surfaceWrapper = CreateScope<Surface>(*ctx.vulkanContext, window);
        ctx.deviceWrapper = CreateScope<Device>(*ctx.surfaceWrapper, *ctx.vulkanContext);

        ctx.device = ctx.deviceWrapper->device();
        ctx.physicalDevice = ctx.deviceWrapper->getPhysicalDevice();
        ctx.surface = ctx.surfaceWrapper->surface();
        ctx.graphicsQueue = ctx.deviceWrapper->graphicsQueue();
        ctx.presentQueue = ctx.deviceWrapper->presentQueue();
        ctx.scene = World::GetActiveScene();

        const EnvironmentSettings environment = World::GetEnvironmentSettings();
        ctx.msaaSamples = ChooseMsaaSamples(ctx.deviceWrapper->m_PhysicalDeviceProperties, environment.aaTechnique, environment.msaaSampleCount);

        // create swapchain wrapper which also creates image views, render pass, framebuffers and sync
        VkExtent2D extent = GetValidSwapChainExtent(ctx.window);
        ctx.swapChainWrapper = CreateScope<SwapChain>(*ctx.deviceWrapper, extent);

        ctx.swapChainImageFormat = ctx.swapChainWrapper->getSwapChainImageFormat();
        ctx.swapChainExtent = ctx.swapChainWrapper->getSwapChainExtent();
        ctx.presentRenderPass = ctx.swapChainWrapper->getRenderPass();
        ctx.offscreenWorldPosRoughnessFormat = ctx.deviceWrapper->findSupportedFormat(
            {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        ctx.offscreenAlbedoAoFormat = ctx.deviceWrapper->findSupportedFormat(
            {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        ctx.offscreenLightingColorFormat = ctx.offscreenWorldPosRoughnessFormat;

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(ScenePushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        ctx.materialSetLayout = DescriptorSetLayout::Builder(*ctx.deviceWrapper)
                                    .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                    .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                    .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                    .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                    .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                    .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                    .build();

        LightingRenderSystem::Initialize(ctx);

        ctx.materialDescriptorPool = DescriptorPool::Builder(*ctx.deviceWrapper)
                                         .setMaxSets(2048)
                                         .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2048 * 6)
                                         .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                                         .build();

        RendererInternals::CreateGeometryRenderPass(ctx);
        RendererInternals::CreateLightingRenderPass(ctx);
        RendererInternals::CreateBloomRenderPass(ctx);
        RendererInternals::CreateOffscreenResources(ctx);
        RendererInternals::CreateCompositeResources(ctx);
        EnsureCompositeEnvironmentDescriptors(ctx);

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

        VkDescriptorSetLayout presentSetLayouts[] = {
            ctx.presentSetLayout->getDescriptorSetLayout(),
            ctx.globalSetLayout->getDescriptorSetLayout()};
        lightingLayoutInfo.setLayoutCount = 2;
        lightingLayoutInfo.pSetLayouts = presentSetLayouts;
        result = vkCreatePipelineLayout(ctx.device, &lightingLayoutInfo, nullptr, &ctx.presentPipelineLayout);
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create present pipeline layout");

        VkDescriptorSetLayout bloomSetLayouts[] = {
            ctx.bloomSetLayout->getDescriptorSetLayout(),
            ctx.globalSetLayout->getDescriptorSetLayout()};
        VkPipelineLayoutCreateInfo bloomLayoutInfo{};
        bloomLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        bloomLayoutInfo.setLayoutCount = 2;
        bloomLayoutInfo.pSetLayouts = bloomSetLayouts;
        result = vkCreatePipelineLayout(ctx.device, &bloomLayoutInfo, nullptr, &ctx.bloomPipelineLayout);
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create bloom pipeline layout");

        const PrimitiveMeshData quadMeshData = PrimitiveMeshDataFactory::CreateQuad();
        ctx.quadMesh = CreateRef<Mesh>(*ctx.deviceWrapper, quadMeshData.vertices, quadMeshData.indices);

        const PrimitiveMeshData cubeMeshData = PrimitiveMeshDataFactory::CreateCube();
        ctx.cubeMesh = CreateRef<Mesh>(*ctx.deviceWrapper, cubeMeshData.vertices, cubeMeshData.indices);

        const PrimitiveMeshData sphereMeshData = PrimitiveMeshDataFactory::CreateSphere();
        ctx.sphereMesh = CreateRef<Mesh>(*ctx.deviceWrapper, sphereMeshData.vertices, sphereMeshData.indices);

        ctx.camera = CreateRef<EditorCamera>(70.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 100.0f);
        if (ctx.scene)
        {
            ctx.scene->OnViewportResize(extent.width, extent.height);
        }

        // Load shaders through the library — loaded once, reused across pipeline recreations.
        ctx.shaderLibrary = CreateScope<ShaderLibrary>(*ctx.deviceWrapper);
        ctx.shaderLibrary->Load("textured.vert", PIECE_SHADER_DIR "/textured.vert.spv", Shader::Stage::Vertex);
        ctx.shaderLibrary->Load("textured.frag", PIECE_SHADER_DIR "/textured.frag.spv", Shader::Stage::Fragment);
        ctx.shaderLibrary->Load("lighting_composite.vert", PIECE_SHADER_DIR "/lighting_composite.vert.spv", Shader::Stage::Vertex);
        ctx.shaderLibrary->Load("lighting_composite.frag", PIECE_SHADER_DIR "/lighting_composite.frag.spv", Shader::Stage::Fragment);
        ctx.shaderLibrary->Load("lighting_composite_msaa.frag", PIECE_SHADER_DIR "/lighting_composite_msaa.frag.spv", Shader::Stage::Fragment);
        ctx.shaderLibrary->Load("present.frag", PIECE_SHADER_DIR "/present.frag.spv", Shader::Stage::Fragment);
        ctx.shaderLibrary->Load("bloom_extract.frag", PIECE_SHADER_DIR "/bloom_extract.frag.spv", Shader::Stage::Fragment);
        ctx.shaderLibrary->Load("bloom_blur.frag", PIECE_SHADER_DIR "/bloom_blur.frag.spv", Shader::Stage::Fragment);
        ctx.shaderLibrary->Load("bloom_blur_vertical.frag", PIECE_SHADER_DIR "/bloom_blur_vertical.frag.spv", Shader::Stage::Fragment);

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
        ctx.objectMaterialFlags.clear();
        ctx.boundEnvironmentSignature.clear();
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
        ctx.bloomExtractPipeline.reset();
        ctx.bloomBlurPipeline.reset();
        ctx.bloomVerticalPipeline.reset();
        ctx.presentPipeline.reset();
        RendererInternals::DestroyPipelineLayouts(ctx);
        RendererInternals::DestroyBloomRenderPass(ctx);
        RendererInternals::DestroyLightingRenderPass(ctx);

        CleanupSwapChain();

        for (FrameResources &frame : ctx.frameResources)
        {
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
        ctx.scene = World::GetActiveScene();

        const EnvironmentSettings environment = World::GetEnvironmentSettings();
        const VkSampleCountFlagBits desiredMsaa = ResolveMsaaSamples(ctx.deviceWrapper->m_PhysicalDeviceProperties, environment.aaTechnique, environment.msaaSampleCount);
        if (desiredMsaa != ctx.msaaSamples)
        {
            RecreateSwapChain();
            return;
        }


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
        ctx.lastRenderedImageIndex = imageIndex;

        LightingRenderSystem::UpdatePerFrame(ctx, static_cast<uint32_t>(ctx.currentFrame));

        // Update material descriptors if needed
        {
            bool anyPending = false;
            const bool environmentChanged = ctx.boundEnvironmentSignature != MakeEnvironmentSignature(environment);
            if (ctx.scene)
            {
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

                if (anyPending || environmentChanged)
                {
                    vkDeviceWaitIdle(ctx.device);
                    for (auto entityHandle : view)
                    {
                        const auto &meshRenderer = view.get<MeshRendererComponent>(entityHandle);
                        EnsureObjectMaterialDescriptor(ctx, static_cast<uint32_t>(entityHandle), meshRenderer.materialId, meshRenderer.materialTextures);
                    }
                    EnsureCompositeEnvironmentDescriptors(ctx);
                }
            }
            else if (environmentChanged)
            {
                vkDeviceWaitIdle(ctx.device);
                EnsureCompositeEnvironmentDescriptors(ctx);
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

        result = ctx.swapChainWrapper->submitCommandBuffers(&frameResources.commandBuffer, &imageIndex, frameResources, ctx.renderFinishedSemaphores[imageIndex], ctx.imagesInFlight);

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
                    if (io.WantCaptureMouse)
                    {
                        ctx.camera->cancelMouseInteraction();
                    }
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

    UUID Renderer::ReadEntityIdAtPixel(uint32_t x, uint32_t y)
    {
        if (!s_Context || x >= s_Context->swapChainExtent.width || y >= s_Context->swapChainExtent.height)
        {
            return UUID{0};
        }

        RendererContext &ctx = *s_Context;
        vkDeviceWaitIdle(ctx.device);

        const OffscreenFrameResources &frame = ctx.offscreenFrames[ctx.lastRenderedImageIndex];
        Scope<Buffer> readback = CreateScope<Buffer>(
            *ctx.deviceWrapper,
            sizeof(uint32_t) * 2,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VkCommandBuffer commandBuffer = ctx.deviceWrapper->beginSingleTimeCommands();
        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), 0};
        region.imageExtent = {1, 1, 1};
        vkCmdCopyImageToBuffer(
            commandBuffer,
            frame.entityIdImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            readback->getBuffer(),
            1,
            &region);
        ctx.deviceWrapper->endSingleTimeCommands(commandBuffer);

        uint32_t idParts[2]{};
        readback->read(idParts, sizeof(idParts));
        return UUID{(static_cast<uint64_t>(idParts[1]) << 32u) | idParts[0]};
    }

    EditorCamera& Renderer::GetEditorCamera()
    {
        PIECE_CORE_ASSERT(s_Context && s_Context->camera, "Renderer camera is not initialized");
        return *s_Context->camera;
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
        if (!s_Context)
        {
            return false;
        }

        const glm::vec3 spawnCenter = s_Context->camera
                                          ? s_Context->camera->focalPoint()
                                          : glm::vec3(0.0f);

        LightingSettings lighting = World::GetLightingSettings();
        if (lighting.pointLightCount >= 4) {
            return false;
        }

        const uint32_t newLightIndex = lighting.pointLightCount;
        PointLightSettings defaults = BuildDefaultPointLight(spawnCenter, newLightIndex);
        lighting.pointLights[newLightIndex] = defaults;
        lighting.pointLightCount = newLightIndex + 1;
        World::SetLightingSettings(lighting);
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

        // Render-finished semaphores are per swapchain image (not per frame-in-flight) to prevent
        // reuse while the presentation engine still holds a reference to a previous use.
        const size_t imageCount = ctx.swapChainWrapper->imageCount();
        ctx.renderFinishedSemaphores.resize(imageCount, VK_NULL_HANDLE);
        for (size_t i = 0; i < imageCount; i++)
        {
            if (ctx.renderFinishedSemaphores[i] == VK_NULL_HANDLE)
            {
                result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.renderFinishedSemaphores[i]);
                PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create render-finished semaphore");
            }
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

        // Destroy per-swapchain-image render-finished semaphores.
        for (VkSemaphore &sem : ctx.renderFinishedSemaphores)
        {
            if (sem != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(ctx.device, sem, nullptr);
                sem = VK_NULL_HANDLE;
            }
        }
        ctx.renderFinishedSemaphores.clear();
    }

    void Renderer::RecreateSwapChain()
    {
        RendererContext &ctx = *s_Context;

        vkDeviceWaitIdle(ctx.device);

        CleanupSwapChain();

        ctx.geometryPipeline.reset();
        ctx.lightingPipeline.reset();
        ctx.bloomExtractPipeline.reset();
        ctx.bloomBlurPipeline.reset();
        ctx.bloomVerticalPipeline.reset();
        ctx.presentPipeline.reset();
        RendererInternals::DestroyPipelineLayouts(ctx);
        RendererInternals::DestroyCompositeResources(ctx);
        RendererInternals::DestroyOffscreenResources(ctx);
        RendererInternals::DestroyBloomRenderPass(ctx);
        RendererInternals::DestroyLightingRenderPass(ctx);
        RendererInternals::DestroyGeometryRenderPass(ctx);

        ctx.swapChainWrapper.reset();

        VkExtent2D extent = GetValidSwapChainExtent(ctx.window);
        ctx.swapChainWrapper = CreateScope<SwapChain>(*ctx.deviceWrapper, extent);

        ctx.swapChainImageFormat = ctx.swapChainWrapper->getSwapChainImageFormat();
        ctx.swapChainExtent = ctx.swapChainWrapper->getSwapChainExtent();
        ctx.presentRenderPass = ctx.swapChainWrapper->getRenderPass();
        ctx.offscreenWorldPosRoughnessFormat = ctx.deviceWrapper->findSupportedFormat(
            {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        ctx.offscreenAlbedoAoFormat = ctx.deviceWrapper->findSupportedFormat(
            {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        ctx.offscreenLightingColorFormat = ctx.offscreenWorldPosRoughnessFormat;
        ctx.imagesInFlight.assign(ctx.swapChainWrapper->imageCount(), VK_NULL_HANDLE);
        ctx.objectBoundMaterialSignature.clear();
        ctx.boundEnvironmentSignature.clear();
        if (ctx.scene)
        {
            ctx.scene->OnViewportResize(extent.width, extent.height);
        }

        const EnvironmentSettings environment = World::GetEnvironmentSettings();
        ctx.msaaSamples = ChooseMsaaSamples(ctx.deviceWrapper->m_PhysicalDeviceProperties, environment.aaTechnique, environment.msaaSampleCount);

        RendererInternals::CreateGeometryRenderPass(ctx);
    RendererInternals::CreateLightingRenderPass(ctx);
        RendererInternals::CreateBloomRenderPass(ctx);
        RendererInternals::CreateOffscreenResources(ctx);
        RendererInternals::CreateCompositeResources(ctx);
        EnsureCompositeEnvironmentDescriptors(ctx);

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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

        VkDescriptorSetLayout presentSetLayouts[] = {
            ctx.presentSetLayout->getDescriptorSetLayout(),
            ctx.globalSetLayout->getDescriptorSetLayout()};
        lightingLayoutInfo.setLayoutCount = 2;
        lightingLayoutInfo.pSetLayouts = presentSetLayouts;
        result = vkCreatePipelineLayout(ctx.device, &lightingLayoutInfo, nullptr, &ctx.presentPipelineLayout);
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to recreate present pipeline layout");

        VkDescriptorSetLayout bloomSetLayouts[] = {
            ctx.bloomSetLayout->getDescriptorSetLayout(),
            ctx.globalSetLayout->getDescriptorSetLayout()};
        VkPipelineLayoutCreateInfo bloomLayoutInfo{};
        bloomLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        bloomLayoutInfo.setLayoutCount = 2;
        bloomLayoutInfo.pSetLayouts = bloomSetLayouts;
        result = vkCreatePipelineLayout(ctx.device, &bloomLayoutInfo, nullptr, &ctx.bloomPipelineLayout);
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to recreate bloom pipeline layout");

        RendererInternals::CreateGraphicsPipeline(ctx);
        CreateCommandBuffers();

        if (s_SwapChainRecreatedCallback)
        {
            s_SwapChainRecreatedCallback();
        }
    }

} // namespace Piece
