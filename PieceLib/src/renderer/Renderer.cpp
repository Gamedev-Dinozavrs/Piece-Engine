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
#include <renderer/LightingRenderSystem.h>
#include <renderer/SceneRenderSystem.h>
#include <renderer/UiRenderSystem.h>
#include <scene/EditorCamera.h>
#include <scene/Mesh.h>
#include <scene/RenderObject.h>
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

        constexpr glm::vec3 kDefaultDirectionalDirection{-0.4f, -1.0f, -0.2f};
        constexpr glm::vec3 kDefaultDirectionalColor{1.0f, 0.98f, 0.9f};
        constexpr float kDefaultDirectionalIntensity = 1.2f;

        void DestroyOffscreenResources(RendererContext &ctx)
        {
            for (OffscreenFrameResources &frame : ctx.offscreenFrames)
            {
                if (frame.framebuffer != VK_NULL_HANDLE)
                {
                    vkDestroyFramebuffer(ctx.device, frame.framebuffer, nullptr);
                    frame.framebuffer = VK_NULL_HANDLE;
                }
                if (frame.msaaWorldPosRoughnessImageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(ctx.device, frame.msaaWorldPosRoughnessImageView, nullptr);
                    frame.msaaWorldPosRoughnessImageView = VK_NULL_HANDLE;
                }
                if (frame.msaaAlbedoAoImageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(ctx.device, frame.msaaAlbedoAoImageView, nullptr);
                    frame.msaaAlbedoAoImageView = VK_NULL_HANDLE;
                }
                if (frame.msaaNormalAoImageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(ctx.device, frame.msaaNormalAoImageView, nullptr);
                    frame.msaaNormalAoImageView = VK_NULL_HANDLE;
                }
                if (frame.worldPosRoughnessImageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(ctx.device, frame.worldPosRoughnessImageView, nullptr);
                    frame.worldPosRoughnessImageView = VK_NULL_HANDLE;
                }
                if (frame.albedoAoImageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(ctx.device, frame.albedoAoImageView, nullptr);
                    frame.albedoAoImageView = VK_NULL_HANDLE;
                }
                if (frame.normalAoImageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(ctx.device, frame.normalAoImageView, nullptr);
                    frame.normalAoImageView = VK_NULL_HANDLE;
                }
                if (frame.depthImageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(ctx.device, frame.depthImageView, nullptr);
                    frame.depthImageView = VK_NULL_HANDLE;
                }
                if (frame.msaaWorldPosRoughnessImage != VK_NULL_HANDLE && frame.msaaWorldPosRoughnessAllocation != nullptr)
                {
                    vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.msaaWorldPosRoughnessImage, frame.msaaWorldPosRoughnessAllocation);
                    frame.msaaWorldPosRoughnessImage = VK_NULL_HANDLE;
                    frame.msaaWorldPosRoughnessAllocation = nullptr;
                }
                if (frame.msaaAlbedoAoImage != VK_NULL_HANDLE && frame.msaaAlbedoAoAllocation != nullptr)
                {
                    vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.msaaAlbedoAoImage, frame.msaaAlbedoAoAllocation);
                    frame.msaaAlbedoAoImage = VK_NULL_HANDLE;
                    frame.msaaAlbedoAoAllocation = nullptr;
                }
                if (frame.msaaNormalAoImage != VK_NULL_HANDLE && frame.msaaNormalAoAllocation != nullptr)
                {
                    vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.msaaNormalAoImage, frame.msaaNormalAoAllocation);
                    frame.msaaNormalAoImage = VK_NULL_HANDLE;
                    frame.msaaNormalAoAllocation = nullptr;
                }
                if (frame.worldPosRoughnessImage != VK_NULL_HANDLE && frame.worldPosRoughnessAllocation != nullptr)
                {
                    vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.worldPosRoughnessImage, frame.worldPosRoughnessAllocation);
                    frame.worldPosRoughnessImage = VK_NULL_HANDLE;
                    frame.worldPosRoughnessAllocation = nullptr;
                }
                if (frame.albedoAoImage != VK_NULL_HANDLE && frame.albedoAoAllocation != nullptr)
                {
                    vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.albedoAoImage, frame.albedoAoAllocation);
                    frame.albedoAoImage = VK_NULL_HANDLE;
                    frame.albedoAoAllocation = nullptr;
                }
                if (frame.normalAoImage != VK_NULL_HANDLE && frame.normalAoAllocation != nullptr)
                {
                    vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.normalAoImage, frame.normalAoAllocation);
                    frame.normalAoImage = VK_NULL_HANDLE;
                    frame.normalAoAllocation = nullptr;
                }
                if (frame.depthImage != VK_NULL_HANDLE && frame.depthAllocation != nullptr)
                {
                    vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.depthImage, frame.depthAllocation);
                    frame.depthImage = VK_NULL_HANDLE;
                    frame.depthAllocation = nullptr;
                }
            }
            ctx.offscreenFrames.clear();
        }

        void DestroyCompositeResources(RendererContext &ctx)
        {
            ctx.compositeDescriptorSets.clear();
            ctx.compositeDescriptorPool.reset();
            ctx.compositeSetLayout.reset();

            if (ctx.compositeSampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(ctx.device, ctx.compositeSampler, nullptr);
                ctx.compositeSampler = VK_NULL_HANDLE;
            }
        }

        void DestroyGeometryRenderPass(RendererContext &ctx)
        {
            if (ctx.geometryRenderPass != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(ctx.device, ctx.geometryRenderPass, nullptr);
                ctx.geometryRenderPass = VK_NULL_HANDLE;
            }
        }

        void DestroyPipelineLayouts(RendererContext &ctx)
        {
            if (ctx.geometryPipelineLayout != VK_NULL_HANDLE)
            {
                vkDestroyPipelineLayout(ctx.device, ctx.geometryPipelineLayout, nullptr);
                ctx.geometryPipelineLayout = VK_NULL_HANDLE;
            }
            if (ctx.lightingPipelineLayout != VK_NULL_HANDLE)
            {
                vkDestroyPipelineLayout(ctx.device, ctx.lightingPipelineLayout, nullptr);
                ctx.lightingPipelineLayout = VK_NULL_HANDLE;
            }
        }

        void CreateGeometryRenderPass(RendererContext &ctx)
        {
            if (ctx.msaaSamples == VK_SAMPLE_COUNT_1_BIT)
            {
                VkAttachmentDescription worldPosRoughnessAttachment{};
                worldPosRoughnessAttachment.format = ctx.offscreenWorldPosRoughnessFormat;
                worldPosRoughnessAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                worldPosRoughnessAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                worldPosRoughnessAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                worldPosRoughnessAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                worldPosRoughnessAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                worldPosRoughnessAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                worldPosRoughnessAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkAttachmentDescription albedoAoAttachment{};
                albedoAoAttachment.format = ctx.offscreenAlbedoAoFormat;
                albedoAoAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                albedoAoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                albedoAoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                albedoAoAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                albedoAoAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                albedoAoAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                albedoAoAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkAttachmentDescription depthAttachment{};
                depthAttachment.format = ctx.swapChainWrapper->findDepthFormat();
                depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription normalAoAttachment{};
                normalAoAttachment.format = ctx.offscreenAlbedoAoFormat;
                normalAoAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                normalAoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                normalAoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                normalAoAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                normalAoAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                normalAoAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                normalAoAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                std::array<VkAttachmentReference, 3> colorAttachmentRefs{};
                colorAttachmentRefs[0].attachment = 0;
                colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachmentRefs[1].attachment = 1;
                colorAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachmentRefs[2].attachment = 2;
                colorAttachmentRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentReference depthAttachmentRef{};
                depthAttachmentRef.attachment = 3;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
                subpass.pColorAttachments = colorAttachmentRefs.data();
                subpass.pDepthStencilAttachment = &depthAttachmentRef;

                std::array<VkSubpassDependency, 2> dependencies{};
                dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[0].dstSubpass = 0;
                dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                dependencies[1].srcSubpass = 0;
                dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                std::array<VkAttachmentDescription, 4> attachments = {
                    worldPosRoughnessAttachment,
                    albedoAoAttachment,
                    normalAoAttachment,
                    depthAttachment};

                VkRenderPassCreateInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                renderPassInfo.pAttachments = attachments.data();
                renderPassInfo.subpassCount = 1;
                renderPassInfo.pSubpasses = &subpass;
                renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
                renderPassInfo.pDependencies = dependencies.data();

                VkResult result = vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.geometryRenderPass);
                assert(result == VK_SUCCESS);
                return;
            }

            VkAttachmentDescription worldPosRoughnessAttachment{};
            worldPosRoughnessAttachment.format = ctx.offscreenWorldPosRoughnessFormat;
            worldPosRoughnessAttachment.samples = ctx.msaaSamples;
            worldPosRoughnessAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            worldPosRoughnessAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            worldPosRoughnessAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            worldPosRoughnessAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            worldPosRoughnessAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            worldPosRoughnessAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription albedoAoAttachment{};
            albedoAoAttachment.format = ctx.offscreenAlbedoAoFormat;
            albedoAoAttachment.samples = ctx.msaaSamples;
            albedoAoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            albedoAoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            albedoAoAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            albedoAoAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            albedoAoAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            albedoAoAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = ctx.swapChainWrapper->findDepthFormat();
            depthAttachment.samples = ctx.msaaSamples;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription normalAoAttachment{};
            normalAoAttachment.format = ctx.offscreenAlbedoAoFormat;
            normalAoAttachment.samples = ctx.msaaSamples;
            normalAoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            normalAoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            normalAoAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            normalAoAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            normalAoAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            normalAoAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription worldPosRoughnessAttachmentResolve{};
            worldPosRoughnessAttachmentResolve.format = ctx.offscreenWorldPosRoughnessFormat;
            worldPosRoughnessAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
            worldPosRoughnessAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            worldPosRoughnessAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            worldPosRoughnessAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            worldPosRoughnessAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            worldPosRoughnessAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            worldPosRoughnessAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkAttachmentDescription albedoAoAttachmentResolve{};
            albedoAoAttachmentResolve.format = ctx.offscreenAlbedoAoFormat;
            albedoAoAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
            albedoAoAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            albedoAoAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            albedoAoAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            albedoAoAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            albedoAoAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            albedoAoAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkAttachmentDescription normalAoAttachmentResolve{};
            normalAoAttachmentResolve.format = ctx.offscreenAlbedoAoFormat;
            normalAoAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
            normalAoAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            normalAoAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            normalAoAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            normalAoAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            normalAoAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            normalAoAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            std::array<VkAttachmentReference, 3> colorAttachmentRefs{};
            colorAttachmentRefs[0].attachment = 0;
            colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentRefs[1].attachment = 1;
            colorAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentRefs[2].attachment = 2;
            colorAttachmentRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            std::array<VkAttachmentReference, 3> resolveAttachmentRefs{};
            resolveAttachmentRefs[0].attachment = 4;
            resolveAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveAttachmentRefs[1].attachment = 5;
            resolveAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveAttachmentRefs[2].attachment = 6;
            resolveAttachmentRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = 3;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
            subpass.pColorAttachments = colorAttachmentRefs.data();
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
            subpass.pResolveAttachments = resolveAttachmentRefs.data();

            std::array<VkSubpassDependency, 2> dependencies{};
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            std::array<VkAttachmentDescription, 7> attachments = {
                worldPosRoughnessAttachment,
                albedoAoAttachment,
                normalAoAttachment,
                depthAttachment,
                worldPosRoughnessAttachmentResolve,
                albedoAoAttachmentResolve,
                normalAoAttachmentResolve
            };

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
            renderPassInfo.pDependencies = dependencies.data();

            VkResult result = vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.geometryRenderPass);
            assert(result == VK_SUCCESS);
        }

        void CreateOffscreenResources(RendererContext &ctx)
        {
            const size_t imageCount = ctx.swapChainWrapper->imageCount();
            ctx.offscreenFrames.resize(imageCount);

            for (size_t i = 0; i < imageCount; ++i)
            {
                OffscreenFrameResources &frame = ctx.offscreenFrames[i];

                auto createColorTarget = [&](VkFormat format,
                                             VkSampleCountFlagBits samples,
                                             VkImageUsageFlags usage,
                                             VkImage &outImage,
                                             VmaAllocation &outAllocation,
                                             VkImageView &outView)
                {
                    VkImageCreateInfo imageInfo{};
                    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageInfo.imageType = VK_IMAGE_TYPE_2D;
                    imageInfo.extent.width = ctx.swapChainExtent.width;
                    imageInfo.extent.height = ctx.swapChainExtent.height;
                    imageInfo.extent.depth = 1;
                    imageInfo.mipLevels = 1;
                    imageInfo.arrayLayers = 1;
                    imageInfo.format = format;
                    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    imageInfo.usage = usage;
                    imageInfo.samples = samples;
                    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                    ctx.deviceWrapper->createImageWithInfo(
                        imageInfo,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        outImage,
                        outAllocation);

                    VkImageViewCreateInfo viewInfo{};
                    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    viewInfo.image = outImage;
                    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    viewInfo.format = format;
                    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    viewInfo.subresourceRange.baseMipLevel = 0;
                    viewInfo.subresourceRange.levelCount = 1;
                    viewInfo.subresourceRange.baseArrayLayer = 0;
                    viewInfo.subresourceRange.layerCount = 1;
                    VkResult viewResult = vkCreateImageView(ctx.device, &viewInfo, nullptr, &outView);
                    assert(viewResult == VK_SUCCESS);
                };

                if (ctx.msaaSamples == VK_SAMPLE_COUNT_1_BIT)
                {
                    createColorTarget(
                        ctx.offscreenWorldPosRoughnessFormat,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        frame.worldPosRoughnessImage,
                        frame.worldPosRoughnessAllocation,
                        frame.worldPosRoughnessImageView);

                    createColorTarget(
                        ctx.offscreenAlbedoAoFormat,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        frame.albedoAoImage,
                        frame.albedoAoAllocation,
                        frame.albedoAoImageView);

                    createColorTarget(
                        ctx.offscreenAlbedoAoFormat,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        frame.normalAoImage,
                        frame.normalAoAllocation,
                        frame.normalAoImageView);
                }
                else
                {
                    createColorTarget(
                        ctx.offscreenWorldPosRoughnessFormat,
                        ctx.msaaSamples,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                        frame.msaaWorldPosRoughnessImage,
                        frame.msaaWorldPosRoughnessAllocation,
                        frame.msaaWorldPosRoughnessImageView);

                    createColorTarget(
                        ctx.offscreenAlbedoAoFormat,
                        ctx.msaaSamples,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                        frame.msaaAlbedoAoImage,
                        frame.msaaAlbedoAoAllocation,
                        frame.msaaAlbedoAoImageView);

                    createColorTarget(
                        ctx.offscreenAlbedoAoFormat,
                        ctx.msaaSamples,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                        frame.msaaNormalAoImage,
                        frame.msaaNormalAoAllocation,
                        frame.msaaNormalAoImageView);

                    createColorTarget(
                        ctx.offscreenWorldPosRoughnessFormat,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        frame.worldPosRoughnessImage,
                        frame.worldPosRoughnessAllocation,
                        frame.worldPosRoughnessImageView);

                    createColorTarget(
                        ctx.offscreenAlbedoAoFormat,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        frame.albedoAoImage,
                        frame.albedoAoAllocation,
                        frame.albedoAoImageView);

                    createColorTarget(
                        ctx.offscreenAlbedoAoFormat,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        frame.normalAoImage,
                        frame.normalAoAllocation,
                        frame.normalAoImageView);
                }

                VkResult result = VK_SUCCESS;
                assert(result == VK_SUCCESS);

                VkFormat depthFormat = ctx.swapChainWrapper->findDepthFormat();
                VkImageCreateInfo depthImageInfo{};
                depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
                depthImageInfo.extent.width = ctx.swapChainExtent.width;
                depthImageInfo.extent.height = ctx.swapChainExtent.height;
                depthImageInfo.extent.depth = 1;
                depthImageInfo.mipLevels = 1;
                depthImageInfo.arrayLayers = 1;
                depthImageInfo.format = depthFormat;
                depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                depthImageInfo.samples = ctx.msaaSamples;
                depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                ctx.deviceWrapper->createImageWithInfo(
                    depthImageInfo,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    frame.depthImage,
                    frame.depthAllocation);

                VkImageViewCreateInfo depthViewInfo{};
                depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                depthViewInfo.image = frame.depthImage;
                depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                depthViewInfo.format = depthFormat;
                depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                depthViewInfo.subresourceRange.baseMipLevel = 0;
                depthViewInfo.subresourceRange.levelCount = 1;
                depthViewInfo.subresourceRange.baseArrayLayer = 0;
                depthViewInfo.subresourceRange.layerCount = 1;
                result = vkCreateImageView(ctx.device, &depthViewInfo, nullptr, &frame.depthImageView);
                assert(result == VK_SUCCESS);

                std::array<VkImageView, 7> attachments{};
                uint32_t attachmentCount = 0;
                if (ctx.msaaSamples == VK_SAMPLE_COUNT_1_BIT)
                {
                    attachments = {
                        frame.worldPosRoughnessImageView,
                        frame.albedoAoImageView,
                        frame.normalAoImageView,
                        frame.depthImageView,
                        VK_NULL_HANDLE,
                        VK_NULL_HANDLE,
                        VK_NULL_HANDLE};
                    attachmentCount = 4;
                }
                else
                {
                    attachments = {
                        frame.msaaWorldPosRoughnessImageView,
                        frame.msaaAlbedoAoImageView,
                        frame.msaaNormalAoImageView,
                        frame.depthImageView,
                        frame.worldPosRoughnessImageView,
                        frame.albedoAoImageView,
                        frame.normalAoImageView};
                    attachmentCount = 7;
                }

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = ctx.geometryRenderPass;
                framebufferInfo.attachmentCount = attachmentCount;
                framebufferInfo.pAttachments = attachments.data();
                framebufferInfo.width = ctx.swapChainExtent.width;
                framebufferInfo.height = ctx.swapChainExtent.height;
                framebufferInfo.layers = 1;
                result = vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, &frame.framebuffer);
                assert(result == VK_SUCCESS);
            }
        }

        void CreateCompositeResources(RendererContext &ctx)
        {
            if (ctx.compositeSampler == VK_NULL_HANDLE)
            {
                VkSamplerCreateInfo samplerInfo{};
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_LINEAR;
                samplerInfo.minFilter = VK_FILTER_LINEAR;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.maxAnisotropy = 1.0f;
                samplerInfo.anisotropyEnable = VK_FALSE;
                samplerInfo.minLod = 0.0f;
                samplerInfo.maxLod = 0.0f;
                VkResult result = vkCreateSampler(ctx.device, &samplerInfo, nullptr, &ctx.compositeSampler);
                assert(result == VK_SUCCESS);
            }

            const uint32_t imageCount = static_cast<uint32_t>(ctx.offscreenFrames.size());
            ctx.compositeSetLayout = DescriptorSetLayout::Builder(*ctx.deviceWrapper)
                                         .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                         .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                         .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                         .build();

            ctx.compositeDescriptorPool = DescriptorPool::Builder(*ctx.deviceWrapper)
                                              .setMaxSets(imageCount)
                                              .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount * 3)
                                              .build();

            ctx.compositeDescriptorSets.assign(imageCount, VK_NULL_HANDLE);
            for (uint32_t i = 0; i < imageCount; ++i)
            {
                const bool allocated = ctx.compositeDescriptorPool->allocateDescriptor(
                    ctx.compositeSetLayout->getDescriptorSetLayout(),
                    ctx.compositeDescriptorSets[i]);
                assert(allocated);

                VkDescriptorImageInfo worldPosRoughnessInfo{};
                worldPosRoughnessInfo.sampler = ctx.compositeSampler;
                worldPosRoughnessInfo.imageView = ctx.offscreenFrames[i].worldPosRoughnessImageView;
                worldPosRoughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkDescriptorImageInfo albedoAoInfo{};
                albedoAoInfo.sampler = ctx.compositeSampler;
                albedoAoInfo.imageView = ctx.offscreenFrames[i].albedoAoImageView;
                albedoAoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkDescriptorImageInfo normalAoInfo{};
                normalAoInfo.sampler = ctx.compositeSampler;
                normalAoInfo.imageView = ctx.offscreenFrames[i].normalAoImageView;
                normalAoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                DescriptorWriter(*ctx.compositeSetLayout, *ctx.compositeDescriptorPool)
                    .writeImage(0, &worldPosRoughnessInfo)
                    .writeImage(1, &albedoAoInfo)
                    .writeImage(2, &normalAoInfo)
                    .overwrite(ctx.compositeDescriptorSets[i]);
            }
        }

        VkExtent2D GetValidSwapChainExtent(Window *window)
        {
            assert(window && "Window must not be null");

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

        void EnsureObjectMaterialDescriptor(RendererContext &ctx, const RenderObject &renderObject)
        {
            const uint32_t objectId = renderObject.objectId();
            const MaterialTextures &material = renderObject.materialTextures();
            const std::string materialSignature = MakeMaterialSignature(material);
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
                assert(allocated && "Failed to allocate material descriptor set");
                ctx.objectMaterialDescriptors[objectId] = descriptorSet;
            }

            auto albedoTexture = GetOrCreateTexture(ctx, material.albedoPath);
            auto roughnessTexture = GetOrCreateTexture(ctx, material.roughnessPath);
            auto aoTexture = GetOrCreateTexture(ctx, material.ambientOcclusionPath);

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

        void SpawnPrimitiveAtCameraTarget(const std::shared_ptr<Mesh> &mesh, PrimitiveType primitiveType)
        {
            if (!s_Context || !mesh || !s_Context->camera)
            {
                return;
            }

            RendererContext &ctx = *s_Context;
            const glm::vec3 spawnPosition = ctx.camera->focalPoint();
            auto object = std::make_shared<RenderObject>(mesh, primitiveType, ctx.nextObjectId++, spawnPosition, glm::vec3(0.0f), glm::vec3(1.0f));
            ctx.renderObjects.push_back(object);
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

        std::vector<Vertex> CreateCubeVertices()
        {
            return {
                // Front face (+Z)
                {{-0.5f, -0.5f,  0.5f}, {1,0,0}, {0,0}},
                {{ 0.5f, -0.5f,  0.5f}, {0,1,0}, {1,0}},
                {{ 0.5f,  0.5f,  0.5f}, {0,0,1}, {1,1}},
                {{-0.5f,  0.5f,  0.5f}, {1,1,0}, {0,1}},

                // Back face (-Z)
                {{-0.5f, -0.5f, -0.5f}, {1,0,1}, {0,0}},
                {{ 0.5f, -0.5f, -0.5f}, {0,1,1}, {1,0}},
                {{ 0.5f,  0.5f, -0.5f}, {0.5,0.5,0.5}, {1,1}},
                {{-0.5f,  0.5f, -0.5f}, {0.8,0.2,0.2}, {0,1}}
            };
        }

        std::vector<uint32_t> CreateCubeIndices()
        {
            return {
                0,1,2, 2,3,0, // front
                4,6,5, 4,7,6, // back
                3,2,6, 6,7,3, // top
                4,5,1, 1,0,4, // bottom
                1,5,6, 6,2,1, // right
                4,0,3, 3,7,4  // left
            };
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

            static std::vector<RenderObject *> renderObjectViews;
            renderObjectViews.clear();
            renderObjectViews.reserve(ctx.renderObjects.size());
            for (const auto &renderObject : ctx.renderObjects)
            {
                if (renderObject)
                {
                    renderObjectViews.push_back(renderObject.get());
                }
            }
            frameInfo.renderObjects = &renderObjectViews;

            return frameInfo;
        }

        void RecordCommandBuffer(const FrameInfo &frameInfo)
        {
            RendererContext &ctx = *s_Context;
            VkCommandBuffer commandBuffer = frameInfo.commandBuffer;
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            VkClearValue geometryClearValues[4] = {};
            geometryClearValues[0].color = {{0.0f, 0.0f, 0.0f, -1.0f}};
            geometryClearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
            geometryClearValues[2].color = {{0.5f, 0.5f, 1.0f, 0.0f}};
            geometryClearValues[3].depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo geometryPassInfo{};
            geometryPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            geometryPassInfo.renderPass = ctx.geometryRenderPass;
            geometryPassInfo.framebuffer = ctx.offscreenFrames[frameInfo.imageIndex].framebuffer;
            geometryPassInfo.renderArea.offset = {0, 0};
            geometryPassInfo.renderArea.extent = frameInfo.swapChainExtent;
            geometryPassInfo.clearValueCount = 4;
            geometryPassInfo.pClearValues = geometryClearValues;

            vkCmdBeginRenderPass(commandBuffer, &geometryPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            FrameInfo geometryFrameInfo = frameInfo;
            geometryFrameInfo.renderPass = ctx.geometryRenderPass;
            geometryFrameInfo.framebuffer = ctx.offscreenFrames[frameInfo.imageIndex].framebuffer;
            geometryFrameInfo.pipelineLayout = ctx.geometryPipelineLayout;
            geometryFrameInfo.pipeline = ctx.geometryPipeline.get();
            SceneRenderSystem::Record(geometryFrameInfo);

            vkCmdEndRenderPass(commandBuffer);

            VkClearValue lightingClearValues[2] = {};
            lightingClearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
            lightingClearValues[1].depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo lightingPassInfo{};
            lightingPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            lightingPassInfo.renderPass = ctx.lightingRenderPass;
            lightingPassInfo.framebuffer = ctx.swapChainWrapper->getFrameBuffer(static_cast<int>(frameInfo.imageIndex));
            lightingPassInfo.renderArea.offset = {0, 0};
            lightingPassInfo.renderArea.extent = frameInfo.swapChainExtent;
            lightingPassInfo.clearValueCount = 2;
            lightingPassInfo.pClearValues = lightingClearValues;

            vkCmdBeginRenderPass(commandBuffer, &lightingPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            LightingRenderSystem::RecordComposite(
                commandBuffer,
                frameInfo.swapChainExtent,
                *ctx.lightingPipeline,
                ctx.lightingPipelineLayout,
                ctx.compositeDescriptorSets[frameInfo.imageIndex],
                ctx.globalDescriptorSets[ctx.currentFrame]);
            UiRenderSystem::Record(commandBuffer);

            vkCmdEndRenderPass(commandBuffer);

            vkEndCommandBuffer(commandBuffer);
        }

        static void CreateGraphicsPipeline()
        {
            RendererContext &ctx = *s_Context;

            PipelineConfigInfo geometryConfig{};
            Pipeline::defaultPipelineConfigInfo(geometryConfig);
            geometryConfig.renderPass = ctx.geometryRenderPass;
            geometryConfig.pipelineLayout = ctx.geometryPipelineLayout;
            geometryConfig.colorBlendAttachments = {
                geometryConfig.colorBlendAttachments[0],
                geometryConfig.colorBlendAttachments[0],
                geometryConfig.colorBlendAttachments[0]};
            geometryConfig.colorBlendInfo.attachmentCount = static_cast<uint32_t>(geometryConfig.colorBlendAttachments.size());
            geometryConfig.colorBlendInfo.pAttachments = geometryConfig.colorBlendAttachments.data();
            geometryConfig.multisampleInfo.rasterizationSamples = ctx.msaaSamples;

            const bool useSampleRateShading =
                (ctx.msaaSamples != VK_SAMPLE_COUNT_1_BIT) &&
                ctx.deviceWrapper->isSampleRateShadingEnabled();
            geometryConfig.multisampleInfo.sampleShadingEnable = useSampleRateShading ? VK_TRUE : VK_FALSE;
            geometryConfig.multisampleInfo.minSampleShading = useSampleRateShading ? 1.0f : 0.0f;

            ctx.geometryPipeline = std::make_unique<Pipeline>(
                *ctx.deviceWrapper,
                *ctx.shaderLibrary->Get("textured.vert"),
                *ctx.shaderLibrary->Get("textured.frag"),
                geometryConfig);

            PipelineConfigInfo lightingConfig{};
            Pipeline::defaultPipelineConfigInfo(lightingConfig);
            lightingConfig.renderPass = ctx.lightingRenderPass;
            lightingConfig.pipelineLayout = ctx.lightingPipelineLayout;
            lightingConfig.bindingDescriptions.clear();
            lightingConfig.attributeDescriptions.clear();
            lightingConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
            lightingConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

            ctx.lightingPipeline = std::make_unique<Pipeline>(
                *ctx.deviceWrapper,
                *ctx.shaderLibrary->Get("lighting_composite.vert"),
                *ctx.shaderLibrary->Get("lighting_composite.frag"),
                lightingConfig);
        }

        VkSampleCountFlagBits ChooseMsaaSamples(const VkPhysicalDeviceProperties& props)
        {
            VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts &
            props.limits.framebufferDepthSampleCounts;
            
            std::cout << "Available MSAA sample counts: " << counts << std::endl;                        
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
        assert(window && "Window must not be null");
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

        CreateGeometryRenderPass(ctx);
        CreateOffscreenResources(ctx);
        CreateCompositeResources(ctx);

        std::vector<VkDescriptorSetLayout> geometrySetLayouts{
            ctx.materialSetLayout->getDescriptorSetLayout(),
            ctx.globalSetLayout->getDescriptorSetLayout()};
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(geometrySetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = geometrySetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        VkResult result = vkCreatePipelineLayout(ctx.device, &pipelineLayoutInfo, nullptr, &ctx.geometryPipelineLayout);
        assert(result == VK_SUCCESS);

        std::vector<VkDescriptorSetLayout> lightingSetLayouts{
            ctx.compositeSetLayout->getDescriptorSetLayout(),
            ctx.globalSetLayout->getDescriptorSetLayout()};
        VkPipelineLayoutCreateInfo lightingLayoutInfo{};
        lightingLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        lightingLayoutInfo.setLayoutCount = static_cast<uint32_t>(lightingSetLayouts.size());
        lightingLayoutInfo.pSetLayouts = lightingSetLayouts.data();
        result = vkCreatePipelineLayout(ctx.device, &lightingLayoutInfo, nullptr, &ctx.lightingPipelineLayout);
        assert(result == VK_SUCCESS);

        auto quadVertices = std::vector<Vertex>{
            {{-0.5f, -0.5f, 0.0f}, {1.0f,0.2f,0.2f}, {0,0}},
            {{ 0.5f, -0.5f, 0.0f}, {0.2f,1.0f,0.2f}, {1,0}},
            {{ 0.5f,  0.5f, 0.0f}, {0.2f,0.2f,1.0f}, {1,1}},
            {{-0.5f,  0.5f, 0.0f}, {1.0f,1.0f,0.2f}, {0,1}}
        };
        auto quadIndices = std::vector<uint32_t>{
            0, 1, 2, 2, 3, 0
        };
        ctx.quadMesh = std::make_shared<Mesh>(*ctx.deviceWrapper, quadVertices, quadIndices);

        auto cubeVertices = CreateCubeVertices();
        auto cubeIndices = CreateCubeIndices();
        ctx.cubeMesh = std::make_shared<Mesh>(*ctx.deviceWrapper, cubeVertices, cubeIndices);

        ctx.camera = std::make_shared<EditorCamera>(70.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 100.0f);

        // Load shaders through the library — loaded once, reused across pipeline recreations.
        ctx.shaderLibrary = std::make_unique<ShaderLibrary>(*ctx.deviceWrapper);
        ctx.shaderLibrary->Load("textured.vert", PIECE_SHADER_DIR "/textured.vert.spv", Shader::Stage::Vertex);
        ctx.shaderLibrary->Load("textured.frag", PIECE_SHADER_DIR "/textured.frag.spv", Shader::Stage::Fragment);
        ctx.shaderLibrary->Load("lighting_composite.vert", PIECE_SHADER_DIR "/lighting_composite.vert.spv", Shader::Stage::Vertex);
        ctx.shaderLibrary->Load("lighting_composite.frag", PIECE_SHADER_DIR "/lighting_composite.frag.spv", Shader::Stage::Fragment);

        CreateGraphicsPipeline();
        // command pool is created by Device; get it
        ctx.commandPool = ctx.deviceWrapper->getCommandPool();
        CreateCommandBuffers();
        ctx.imagesInFlight.assign(ctx.swapChainWrapper->imageCount(), VK_NULL_HANDLE);

        std::cout << ctx.msaaSamples << "x MSAA enabled" << std::endl;
    }

    void Renderer::Shutdown()
    {
        if (!s_Context || s_Context->device == VK_NULL_HANDLE)
        {
            return;
        }

        RendererContext &ctx = *s_Context;

        vkDeviceWaitIdle(ctx.device);

        ctx.renderObjects.clear();
        ctx.quadMesh.reset();
        ctx.cubeMesh.reset();
        ctx.camera.reset();
        ctx.objectMaterialDescriptors.clear();
        ctx.objectBoundMaterialSignature.clear();
        ctx.textureCache.clear();
        LightingRenderSystem::Shutdown(ctx);
        DestroyCompositeResources(ctx);
        DestroyOffscreenResources(ctx);
        DestroyGeometryRenderPass(ctx);
        ctx.materialDescriptorPool.reset();
        ctx.materialSetLayout.reset();
        ctx.shaderLibrary.reset();

        ctx.geometryPipeline.reset();
        ctx.lightingPipeline.reset();
        DestroyPipelineLayouts(ctx);

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
        assert(s_Context && "Renderer context is not initialized");
        RendererContext &ctx = *s_Context;

        uint32_t imageIndex;
        FrameResources &frameResources = ctx.frameResources[ctx.currentFrame];
        VkResult result = ctx.swapChainWrapper->acquireNextImage(&imageIndex, frameResources);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapChain();
            return;
        }
        assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

        frameResources.imageIndex = imageIndex;

        LightingRenderSystem::UpdatePerFrame(ctx, static_cast<uint32_t>(ctx.currentFrame));

        // Update material descriptors outside command buffer recording.
        // vkDeviceWaitIdle is only called when at least one object needs an update.
        {
            bool anyPending = false;
            for (const auto &ro : ctx.renderObjects)
            {
                if (!ro)
                    continue;
                uint32_t id = ro->objectId();
                bool noDescriptor = ctx.objectMaterialDescriptors.find(id) == ctx.objectMaterialDescriptors.end();
                bool pathChanged = ctx.objectBoundMaterialSignature[id] != MakeMaterialSignature(ro->materialTextures());
                if (noDescriptor || pathChanged)
                {
                    anyPending = true;
                    break;
                }
            }
            if (anyPending)
            {
                vkDeviceWaitIdle(ctx.device);
                for (const auto &ro : ctx.renderObjects)
                {
                    if (ro)
                        EnsureObjectMaterialDescriptor(ctx, *ro);
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
        assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

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
        assert(s_Context && s_Context->deviceWrapper && "Renderer device is not initialized");
        return *s_Context->deviceWrapper;
    }

    RenderPass &Renderer::GetRenderPass()
    {
        assert(s_Context && s_Context->swapChainWrapper && "Renderer swapchain is not initialized");
        return s_Context->swapChainWrapper->getRenderPassObject();
    }

    VulkanContext &Renderer::GetVulkanContext()
    {
        assert(s_Context && s_Context->vulkanContext && "Renderer Vulkan context is not initialized");
        return *s_Context->vulkanContext;
    }

    void Renderer::SetSwapChainRecreatedCallback(const std::function<void()> &callback)
    {
        s_SwapChainRecreatedCallback = callback;
    }

    LightingSettings Renderer::GetLightingSettings()
    {
        return LightingRenderSystem::GetSettings();
    }

    void Renderer::SetLightingSettings(const LightingSettings &settings)
    {
        LightingRenderSystem::SetSettings(settings);
    }

    bool Renderer::CreatePointLightInView()
    {
        if (!s_Context)
        {
            return false;
        }

        LightingSettings lighting = LightingRenderSystem::GetSettings();
        if (lighting.pointLightCount >= lighting.pointLights.size())
        {
            return false;
        }

        const glm::vec3 spawnCenter = s_Context->camera
                                          ? s_Context->camera->focalPoint()
                                          : glm::vec3(0.0f);

        const uint32_t nextIndex = lighting.pointLightCount;
        lighting.pointLights[nextIndex] = BuildDefaultPointLight(spawnCenter, nextIndex);
        lighting.pointLightCount = nextIndex + 1;
        LightingRenderSystem::SetSettings(lighting);
        return true;
    }

    void Renderer::ResetDirectionalLight()
    {
        LightingSettings lighting = LightingRenderSystem::GetSettings();
        lighting.directionalDirection = kDefaultDirectionalDirection;
        lighting.directionalColor = kDefaultDirectionalColor;
        lighting.directionalIntensity = kDefaultDirectionalIntensity;
        LightingRenderSystem::SetSettings(lighting);
    }

    void Renderer::CreateQuadInView()
    {
        if (!s_Context)
        {
            return;
        }

        SpawnPrimitiveAtCameraTarget(s_Context->quadMesh, PrimitiveType::Quad);
    }

    void Renderer::CreateCubeInView()
    {
        if (!s_Context)
        {
            return;
        }

        SpawnPrimitiveAtCameraTarget(s_Context->cubeMesh, PrimitiveType::Cube);
    }

    std::vector<QuadMaterialView> Renderer::GetQuadMaterials()
    {
        std::vector<QuadMaterialView> quads;
        if (!s_Context)
        {
            return quads;
        }

        for (const auto &renderObject : s_Context->renderObjects)
        {
            if (!renderObject || renderObject->primitiveType() != PrimitiveType::Quad)
            {
                continue;
            }

            const auto &material = renderObject->materialTextures();
            QuadMaterialView quad{};
            quad.id = renderObject->objectId();
            quad.albedoPath = material.albedoPath;
            quad.normalPath = material.normalPath;
            quad.heightPath = material.heightPath;
            quad.roughnessPath = material.roughnessPath;
            quad.ambientOcclusionPath = material.ambientOcclusionPath;
            quads.push_back(std::move(quad));
        }

        return quads;
    }

    bool Renderer::SetQuadTexturePath(uint32_t quadId, TextureSlot slot, const std::string &path)
    {
        if (!s_Context)
        {
            return false;
        }

        for (const auto &renderObject : s_Context->renderObjects)
        {
            if (!renderObject || renderObject->primitiveType() != PrimitiveType::Quad || renderObject->objectId() != quadId)
            {
                continue;
            }

            auto &material = renderObject->materialTextures();
            switch (slot)
            {
            case TextureSlot::Albedo:
                material.albedoPath = path;
                return true;
            case TextureSlot::Normal:
                material.normalPath = path;
                return true;
            case TextureSlot::Height:
                material.heightPath = path;
                return true;
            case TextureSlot::Roughness:
                material.roughnessPath = path;
                return true;
            case TextureSlot::AmbientOcclusion:
                material.ambientOcclusionPath = path;
                return true;
            default:
                return false;
            }
        }

        return false;
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
        assert(result == VK_SUCCESS);

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
                assert(result == VK_SUCCESS);
            }
            if (ctx.frameResources[i].renderFinishedSemaphore == VK_NULL_HANDLE)
            {
                result = vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.frameResources[i].renderFinishedSemaphore);
                assert(result == VK_SUCCESS);
            }
            if (ctx.frameResources[i].inFlightFence == VK_NULL_HANDLE)
            {
                result = vkCreateFence(ctx.device, &fenceInfo, nullptr, &ctx.frameResources[i].inFlightFence);
                assert(result == VK_SUCCESS);
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
        DestroyPipelineLayouts(ctx);
        DestroyCompositeResources(ctx);
        DestroyOffscreenResources(ctx);
        DestroyGeometryRenderPass(ctx);

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

        ctx.msaaSamples = ChooseMsaaSamples(ctx.deviceWrapper->properties);

        CreateGeometryRenderPass(ctx);
        CreateOffscreenResources(ctx);
        CreateCompositeResources(ctx);

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
        assert(result == VK_SUCCESS);

        std::vector<VkDescriptorSetLayout> lightingSetLayouts{
            ctx.compositeSetLayout->getDescriptorSetLayout(),
            ctx.globalSetLayout->getDescriptorSetLayout()};
        VkPipelineLayoutCreateInfo lightingLayoutInfo{};
        lightingLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        lightingLayoutInfo.setLayoutCount = static_cast<uint32_t>(lightingSetLayouts.size());
        lightingLayoutInfo.pSetLayouts = lightingSetLayouts.data();
        result = vkCreatePipelineLayout(ctx.device, &lightingLayoutInfo, nullptr, &ctx.lightingPipelineLayout);
        assert(result == VK_SUCCESS);

        CreateGraphicsPipeline();
        CreateCommandBuffers();

        if (s_SwapChainRecreatedCallback)
        {
            s_SwapChainRecreatedCallback();
        }
    }

} // namespace Piece
