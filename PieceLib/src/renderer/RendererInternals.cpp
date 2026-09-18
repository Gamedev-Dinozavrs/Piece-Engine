#include <PiecePCH.h>

#include <renderer/RendererInternals.h>

#include <renderer/Descriptors.h>
#include <renderer/Device.h>
#include <renderer/Pipeline.h>
#include <renderer/ShaderLibrary.h>
#include <renderer/SwapChain.h>
#include <renderer/Texture.h>
#include <scene/World.h>

#include <array>
#include <filesystem>
#include <string>

namespace Piece {

namespace RendererInternals {

void DestroyOffscreenResources(RendererContext &ctx)
{
    for (OffscreenFrameResources &frame : ctx.offscreenFrames)
    {
        if (frame.lightingFramebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(ctx.device, frame.lightingFramebuffer, nullptr);
            frame.lightingFramebuffer = VK_NULL_HANDLE;
        }
        if (frame.bloomExtractFramebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(ctx.device, frame.bloomExtractFramebuffer, nullptr);
            frame.bloomExtractFramebuffer = VK_NULL_HANDLE;
        }
        if (frame.bloomBlurFramebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(ctx.device, frame.bloomBlurFramebuffer, nullptr);
            frame.bloomBlurFramebuffer = VK_NULL_HANDLE;
        }
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
        if (frame.msaaEmissiveImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, frame.msaaEmissiveImageView, nullptr);
            frame.msaaEmissiveImageView = VK_NULL_HANDLE;
        }
        if (frame.msaaBloomParamsImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, frame.msaaBloomParamsImageView, nullptr);
            frame.msaaBloomParamsImageView = VK_NULL_HANDLE;
        }
        if (frame.msaaEntityIdImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, frame.msaaEntityIdImageView, nullptr);
            frame.msaaEntityIdImageView = VK_NULL_HANDLE;
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
        if (frame.emissiveImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, frame.emissiveImageView, nullptr);
            frame.emissiveImageView = VK_NULL_HANDLE;
        }
        if (frame.bloomParamsImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, frame.bloomParamsImageView, nullptr);
            frame.bloomParamsImageView = VK_NULL_HANDLE;
        }
        if (frame.entityIdImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, frame.entityIdImageView, nullptr);
            frame.entityIdImageView = VK_NULL_HANDLE;
        }
        if (frame.msaaLightingColorImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, frame.msaaLightingColorImageView, nullptr);
            frame.msaaLightingColorImageView = VK_NULL_HANDLE;
        }
        if (frame.lightingColorImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, frame.lightingColorImageView, nullptr);
            frame.lightingColorImageView = VK_NULL_HANDLE;
        }
        if (frame.bloomExtractImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, frame.bloomExtractImageView, nullptr);
            frame.bloomExtractImageView = VK_NULL_HANDLE;
        }
        if (frame.bloomBlurImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, frame.bloomBlurImageView, nullptr);
            frame.bloomBlurImageView = VK_NULL_HANDLE;
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
        if (frame.msaaEmissiveImage != VK_NULL_HANDLE && frame.msaaEmissiveAllocation != nullptr)
        {
            vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.msaaEmissiveImage, frame.msaaEmissiveAllocation);
            frame.msaaEmissiveImage = VK_NULL_HANDLE;
            frame.msaaEmissiveAllocation = nullptr;
        }
        if (frame.msaaBloomParamsImage != VK_NULL_HANDLE && frame.msaaBloomParamsAllocation != nullptr)
        {
            vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.msaaBloomParamsImage, frame.msaaBloomParamsAllocation);
            frame.msaaBloomParamsImage = VK_NULL_HANDLE;
            frame.msaaBloomParamsAllocation = nullptr;
        }
        if (frame.msaaEntityIdImage != VK_NULL_HANDLE && frame.msaaEntityIdAllocation != nullptr)
        {
            vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.msaaEntityIdImage, frame.msaaEntityIdAllocation);
            frame.msaaEntityIdImage = VK_NULL_HANDLE;
            frame.msaaEntityIdAllocation = nullptr;
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
        if (frame.emissiveImage != VK_NULL_HANDLE && frame.emissiveAllocation != nullptr)
        {
            vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.emissiveImage, frame.emissiveAllocation);
            frame.emissiveImage = VK_NULL_HANDLE;
            frame.emissiveAllocation = nullptr;
        }
        if (frame.bloomParamsImage != VK_NULL_HANDLE && frame.bloomParamsAllocation != nullptr)
        {
            vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.bloomParamsImage, frame.bloomParamsAllocation);
            frame.bloomParamsImage = VK_NULL_HANDLE;
            frame.bloomParamsAllocation = nullptr;
        }
        if (frame.entityIdImage != VK_NULL_HANDLE && frame.entityIdAllocation != nullptr)
        {
            vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.entityIdImage, frame.entityIdAllocation);
            frame.entityIdImage = VK_NULL_HANDLE;
            frame.entityIdAllocation = nullptr;
        }
        if (frame.msaaLightingColorImage != VK_NULL_HANDLE && frame.msaaLightingColorAllocation != nullptr)
        {
            vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.msaaLightingColorImage, frame.msaaLightingColorAllocation);
            frame.msaaLightingColorImage = VK_NULL_HANDLE;
            frame.msaaLightingColorAllocation = nullptr;
        }
        if (frame.lightingColorImage != VK_NULL_HANDLE && frame.lightingColorAllocation != nullptr)
        {
            vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.lightingColorImage, frame.lightingColorAllocation);
            frame.lightingColorImage = VK_NULL_HANDLE;
            frame.lightingColorAllocation = nullptr;
        }
        if (frame.bloomExtractImage != VK_NULL_HANDLE && frame.bloomExtractAllocation != nullptr)
        {
            vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.bloomExtractImage, frame.bloomExtractAllocation);
            frame.bloomExtractImage = VK_NULL_HANDLE;
            frame.bloomExtractAllocation = nullptr;
        }
        if (frame.bloomBlurImage != VK_NULL_HANDLE && frame.bloomBlurAllocation != nullptr)
        {
            vmaDestroyImage(ctx.deviceWrapper->allocator(), frame.bloomBlurImage, frame.bloomBlurAllocation);
            frame.bloomBlurImage = VK_NULL_HANDLE;
            frame.bloomBlurAllocation = nullptr;
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
    ctx.presentDescriptorSets.clear();
    ctx.presentDescriptorPool.reset();
    ctx.presentSetLayout.reset();
    ctx.bloomExtractDescriptorSets.clear();
    ctx.bloomBlurDescriptorSets.clear();
    ctx.bloomVerticalDescriptorSets.clear();
    ctx.bloomDescriptorPool.reset();
    ctx.bloomSetLayout.reset();

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

void DestroyLightingRenderPass(RendererContext &ctx)
{
    if (ctx.lightingRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(ctx.device, ctx.lightingRenderPass, nullptr);
        ctx.lightingRenderPass = VK_NULL_HANDLE;
    }
}

void DestroyBloomRenderPass(RendererContext &ctx)
{
    if (ctx.bloomRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(ctx.device, ctx.bloomRenderPass, nullptr);
        ctx.bloomRenderPass = VK_NULL_HANDLE;
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
    if (ctx.presentPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(ctx.device, ctx.presentPipelineLayout, nullptr);
        ctx.presentPipelineLayout = VK_NULL_HANDLE;
    }
    if (ctx.bloomPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(ctx.device, ctx.bloomPipelineLayout, nullptr);
        ctx.bloomPipelineLayout = VK_NULL_HANDLE;
    }
    if (ctx.billboardPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(ctx.device, ctx.billboardPipelineLayout, nullptr);
        ctx.billboardPipelineLayout = VK_NULL_HANDLE;
    }
}

void DestroyBillboardResources(RendererContext &ctx)
{
    ctx.pointLightIconDescriptorSet = VK_NULL_HANDLE;
    ctx.billboardDescriptorPool.reset();
    ctx.billboardSetLayout.reset();
    ctx.pointLightIconTexture.reset();
}

void CreateLightingRenderPass(RendererContext &ctx)
{
    if (ctx.msaaSamples == VK_SAMPLE_COUNT_1_BIT)
    {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = ctx.offscreenLightingColorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

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

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VkResult result = vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.lightingRenderPass);
    PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create lighting render pass");
        return;
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = ctx.offscreenLightingColorFormat;
    colorAttachment.samples = ctx.msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription resolveAttachment{};
    resolveAttachment.format = ctx.offscreenLightingColorFormat;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolveAttachmentRef{};
    resolveAttachmentRef.attachment = 1;
    resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pResolveAttachments = &resolveAttachmentRef;

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

    std::array<VkAttachmentDescription, 2> attachments = {
        colorAttachment,
        resolveAttachment
    };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VkResult result = vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.lightingRenderPass);
    PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create MSAA lighting render pass");
}

void CreateBloomRenderPass(RendererContext &ctx)
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = ctx.offscreenLightingColorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;

    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    const VkResult result = vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.bloomRenderPass);
    PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create bloom render pass");
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

        VkAttachmentDescription emissiveAttachment{};
        emissiveAttachment.format = ctx.offscreenAlbedoAoFormat;
        emissiveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        emissiveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        emissiveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        emissiveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        emissiveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        emissiveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        emissiveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentDescription bloomParamsAttachment = emissiveAttachment;
        bloomParamsAttachment.format = ctx.bloomParamsFormat;

        std::array<VkAttachmentReference, 4> colorAttachmentRefs{};
        colorAttachmentRefs[0].attachment = 0;
        colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs[1].attachment = 1;
        colorAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs[2].attachment = 2;
        colorAttachmentRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs[3].attachment = 3;
        colorAttachmentRefs[3].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 5;
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

        VkAttachmentDescription entityIdAttachment{};
        entityIdAttachment.format = ctx.entityIdFormat;
        entityIdAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        entityIdAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        entityIdAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        entityIdAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        entityIdAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        entityIdAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        entityIdAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        VkAttachmentReference bloomParamsAttachmentRef{};
        bloomParamsAttachmentRef.attachment = 4;
        bloomParamsAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference entityIdAttachmentRef{};
        entityIdAttachmentRef.attachment = 6;
        entityIdAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        subpass.colorAttachmentCount = 6;
        std::array<VkAttachmentReference, 6> singleSampleColorRefs{
            colorAttachmentRefs[0], colorAttachmentRefs[1], colorAttachmentRefs[2], colorAttachmentRefs[3], entityIdAttachmentRef, bloomParamsAttachmentRef};
        subpass.pColorAttachments = singleSampleColorRefs.data();

        std::array<VkAttachmentDescription, 7> attachments = {
            worldPosRoughnessAttachment,
            albedoAoAttachment,
            normalAoAttachment,
            emissiveAttachment,
            bloomParamsAttachment,
            depthAttachment,
            entityIdAttachment};

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        VkResult result = vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.geometryRenderPass);
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create geometry render pass");
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
    worldPosRoughnessAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription albedoAoAttachment{};
    albedoAoAttachment.format = ctx.offscreenAlbedoAoFormat;
    albedoAoAttachment.samples = ctx.msaaSamples;
    albedoAoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    albedoAoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    albedoAoAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    albedoAoAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    albedoAoAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    albedoAoAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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
    normalAoAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription emissiveAttachment{};
    emissiveAttachment.format = ctx.offscreenAlbedoAoFormat;
    emissiveAttachment.samples = ctx.msaaSamples;
    emissiveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    emissiveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    emissiveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    emissiveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    emissiveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    emissiveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription bloomParamsAttachment = emissiveAttachment;
    bloomParamsAttachment.format = ctx.bloomParamsFormat;

    VkAttachmentDescription entityIdAttachment{};
    entityIdAttachment.format = ctx.entityIdFormat;
    entityIdAttachment.samples = ctx.msaaSamples;
    entityIdAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    entityIdAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    entityIdAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    entityIdAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    entityIdAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    entityIdAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription entityIdAttachmentResolve{};
    entityIdAttachmentResolve.format = ctx.entityIdFormat;
    entityIdAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    entityIdAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    entityIdAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    entityIdAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    entityIdAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    entityIdAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    entityIdAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

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

    VkAttachmentDescription emissiveAttachmentResolve{};
    emissiveAttachmentResolve.format = ctx.offscreenAlbedoAoFormat;
    emissiveAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    emissiveAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    emissiveAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    emissiveAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    emissiveAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    emissiveAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    emissiveAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription bloomParamsAttachmentResolve = emissiveAttachmentResolve;
    bloomParamsAttachmentResolve.format = ctx.bloomParamsFormat;

    std::array<VkAttachmentReference, 6> colorAttachmentRefs{};
    colorAttachmentRefs[0].attachment = 0;
    colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentRefs[1].attachment = 1;
    colorAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentRefs[2].attachment = 2;
    colorAttachmentRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentRefs[3].attachment = 3;
    colorAttachmentRefs[3].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentRefs[4].attachment = 6;
    colorAttachmentRefs[4].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentRefs[5].attachment = 4;
    colorAttachmentRefs[5].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 6> resolveAttachmentRefs{};
    resolveAttachmentRefs[0].attachment = 7;
    resolveAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolveAttachmentRefs[1].attachment = 8;
    resolveAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolveAttachmentRefs[2].attachment = 9;
    resolveAttachmentRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolveAttachmentRefs[3].attachment = 10;
    resolveAttachmentRefs[3].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolveAttachmentRefs[4].attachment = 12;
    resolveAttachmentRefs[4].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolveAttachmentRefs[5].attachment = 11;
    resolveAttachmentRefs[5].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 5;
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

    std::array<VkAttachmentDescription, 13> attachments = {
        worldPosRoughnessAttachment,
        albedoAoAttachment,
        normalAoAttachment,
        emissiveAttachment,
        bloomParamsAttachment,
        depthAttachment,
        entityIdAttachment,
        worldPosRoughnessAttachmentResolve,
        albedoAoAttachmentResolve,
        normalAoAttachmentResolve,
        emissiveAttachmentResolve,
        bloomParamsAttachmentResolve,
        entityIdAttachmentResolve
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
    PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create MSAA geometry render pass");
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
            PIECE_CORE_ASSERT(viewResult == VK_SUCCESS, "Failed to create offscreen color image view");
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

            createColorTarget(
                ctx.offscreenAlbedoAoFormat,
                VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                frame.emissiveImage,
                frame.emissiveAllocation,
                frame.emissiveImageView);

            createColorTarget(
                ctx.bloomParamsFormat,
                VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                frame.bloomParamsImage,
                frame.bloomParamsAllocation,
                frame.bloomParamsImageView);

            createColorTarget(
                ctx.entityIdFormat,
                VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                frame.entityIdImage,
                frame.entityIdAllocation,
                frame.entityIdImageView);
        }
        else
        {
            createColorTarget(
                ctx.offscreenWorldPosRoughnessFormat,
                ctx.msaaSamples,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                frame.msaaWorldPosRoughnessImage,
                frame.msaaWorldPosRoughnessAllocation,
                frame.msaaWorldPosRoughnessImageView);

            createColorTarget(
                ctx.offscreenAlbedoAoFormat,
                ctx.msaaSamples,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                frame.msaaAlbedoAoImage,
                frame.msaaAlbedoAoAllocation,
                frame.msaaAlbedoAoImageView);

            createColorTarget(
                ctx.offscreenAlbedoAoFormat,
                ctx.msaaSamples,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                frame.msaaNormalAoImage,
                frame.msaaNormalAoAllocation,
                frame.msaaNormalAoImageView);

            createColorTarget(
                ctx.offscreenAlbedoAoFormat,
                ctx.msaaSamples,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                frame.msaaEmissiveImage,
                frame.msaaEmissiveAllocation,
                frame.msaaEmissiveImageView);

            createColorTarget(
                ctx.bloomParamsFormat,
                ctx.msaaSamples,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                frame.msaaBloomParamsImage,
                frame.msaaBloomParamsAllocation,
                frame.msaaBloomParamsImageView);

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

            createColorTarget(
                ctx.offscreenAlbedoAoFormat,
                VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                frame.emissiveImage,
                frame.emissiveAllocation,
                frame.emissiveImageView);

            createColorTarget(
                ctx.bloomParamsFormat,
                VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                frame.bloomParamsImage,
                frame.bloomParamsAllocation,
                frame.bloomParamsImageView);

            createColorTarget(
                ctx.entityIdFormat,
                ctx.msaaSamples,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                frame.msaaEntityIdImage,
                frame.msaaEntityIdAllocation,
                frame.msaaEntityIdImageView);

            createColorTarget(
                ctx.entityIdFormat,
                VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                frame.entityIdImage,
                frame.entityIdAllocation,
                frame.entityIdImageView);
        }

        VkResult result = VK_SUCCESS;
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Unexpected offscreen setup failure");

        createColorTarget(
            ctx.offscreenLightingColorFormat,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            frame.lightingColorImage,
            frame.lightingColorAllocation,
            frame.lightingColorImageView);

        if (ctx.msaaSamples != VK_SAMPLE_COUNT_1_BIT)
        {
            createColorTarget(
                ctx.offscreenLightingColorFormat,
                ctx.msaaSamples,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                frame.msaaLightingColorImage,
                frame.msaaLightingColorAllocation,
                frame.msaaLightingColorImageView);
        }

        createColorTarget(
            ctx.offscreenLightingColorFormat,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            frame.bloomExtractImage,
            frame.bloomExtractAllocation,
            frame.bloomExtractImageView);
        createColorTarget(
            ctx.offscreenLightingColorFormat,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            frame.bloomBlurImage,
            frame.bloomBlurAllocation,
            frame.bloomBlurImageView);

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
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create offscreen depth image view");

        std::array<VkImageView, 13> attachments{};
        uint32_t attachmentCount = 0;
        if (ctx.msaaSamples == VK_SAMPLE_COUNT_1_BIT)
        {
            attachments = {
                frame.worldPosRoughnessImageView,
                frame.albedoAoImageView,
                frame.normalAoImageView,
                frame.emissiveImageView,
                frame.bloomParamsImageView,
                frame.depthImageView,
                frame.entityIdImageView,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE};
            attachmentCount = 6;
        }
        else
        {
            attachments = {
                frame.msaaWorldPosRoughnessImageView,
                frame.msaaAlbedoAoImageView,
                frame.msaaNormalAoImageView,
                frame.msaaEmissiveImageView,
                frame.msaaBloomParamsImageView,
                frame.depthImageView,
                frame.msaaEntityIdImageView,
                frame.worldPosRoughnessImageView,
                frame.albedoAoImageView,
                frame.normalAoImageView,
                frame.emissiveImageView,
                frame.bloomParamsImageView,
                frame.entityIdImageView};
            attachmentCount = 13;
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
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create offscreen framebuffer");

        VkFramebufferCreateInfo lightingFramebufferInfo{};
        lightingFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        lightingFramebufferInfo.renderPass = ctx.lightingRenderPass;
        lightingFramebufferInfo.width = ctx.swapChainExtent.width;
        lightingFramebufferInfo.height = ctx.swapChainExtent.height;
        lightingFramebufferInfo.layers = 1;

        if (ctx.msaaSamples == VK_SAMPLE_COUNT_1_BIT)
        {
            VkImageView lightingAttachment = frame.lightingColorImageView;
            lightingFramebufferInfo.attachmentCount = 1;
            lightingFramebufferInfo.pAttachments = &lightingAttachment;
            result = vkCreateFramebuffer(ctx.device, &lightingFramebufferInfo, nullptr, &frame.lightingFramebuffer);
            PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create lighting framebuffer");

        }
        else
        {
            std::array<VkImageView, 2> lightingAttachments = {
                frame.msaaLightingColorImageView,
                frame.lightingColorImageView
            };
            lightingFramebufferInfo.attachmentCount = static_cast<uint32_t>(lightingAttachments.size());
            lightingFramebufferInfo.pAttachments = lightingAttachments.data();
            result = vkCreateFramebuffer(ctx.device, &lightingFramebufferInfo, nullptr, &frame.lightingFramebuffer);
            PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create MSAA lighting framebuffer");
        }

        VkFramebufferCreateInfo bloomFramebufferInfo{};
        bloomFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        bloomFramebufferInfo.renderPass = ctx.bloomRenderPass;
        bloomFramebufferInfo.attachmentCount = 1;
        bloomFramebufferInfo.pAttachments = &frame.bloomExtractImageView;
        bloomFramebufferInfo.width = ctx.swapChainExtent.width;
        bloomFramebufferInfo.height = ctx.swapChainExtent.height;
        bloomFramebufferInfo.layers = 1;
        result = vkCreateFramebuffer(ctx.device, &bloomFramebufferInfo, nullptr, &frame.bloomExtractFramebuffer);
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create bloom extraction framebuffer");
        bloomFramebufferInfo.pAttachments = &frame.bloomBlurImageView;
        result = vkCreateFramebuffer(ctx.device, &bloomFramebufferInfo, nullptr, &frame.bloomBlurFramebuffer);
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create bloom blur framebuffer");
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
        PIECE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create composite sampler");
    }

    const uint32_t imageCount = static_cast<uint32_t>(ctx.offscreenFrames.size());
    ctx.compositeSetLayout = DescriptorSetLayout::Builder(*ctx.deviceWrapper)
                                 .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                 .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                 .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                 .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                 .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                 .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                 .build();

    ctx.compositeDescriptorPool = DescriptorPool::Builder(*ctx.deviceWrapper)
                                      .setMaxSets(imageCount)
                                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount * 6)
                                      .build();

    ctx.presentSetLayout = DescriptorSetLayout::Builder(*ctx.deviceWrapper)
                               .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                               .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                               .build();

    ctx.presentDescriptorPool = DescriptorPool::Builder(*ctx.deviceWrapper)
                                    .setMaxSets(imageCount)
                                                                        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount * 2)
                                    .build();

        ctx.bloomSetLayout = DescriptorSetLayout::Builder(*ctx.deviceWrapper)
                                                            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                                            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                                            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                                            .build();
        ctx.bloomDescriptorPool = DescriptorPool::Builder(*ctx.deviceWrapper)
                                                                      .setMaxSets(imageCount * 3)
                                                                          .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount * 9)
                                                                    .build();

    const EnvironmentSettings environment = World::GetEnvironmentSettings();
    const bool useMsaaComposite =
        environment.aaTechnique == AATechnique::MSAA &&
        ctx.msaaSamples != VK_SAMPLE_COUNT_1_BIT;
    auto getOrCreateTexture = [&](const std::string& texturePath) {
        const std::string key = texturePath.empty() ? "__DEFAULT_WHITE__" : texturePath;
        auto it = ctx.textureCache.find(key);
        if (it != ctx.textureCache.end()) {
            return it->second;
        }

        Ref<Texture> texture = CreateRef<Texture>(*ctx.deviceWrapper, texturePath);
        ctx.textureCache[key] = texture;
        return texture;
    };

    Ref<Texture> envDiffuseTexture = getOrCreateTexture(environment.diffuseMapPath);
    Ref<Texture> envSpecularTexture = getOrCreateTexture(environment.specularMapPath);

    ctx.compositeDescriptorSets.assign(imageCount, VK_NULL_HANDLE);
    ctx.presentDescriptorSets.assign(imageCount, VK_NULL_HANDLE);
    ctx.bloomExtractDescriptorSets.assign(imageCount, VK_NULL_HANDLE);
    ctx.bloomBlurDescriptorSets.assign(imageCount, VK_NULL_HANDLE);
    ctx.bloomVerticalDescriptorSets.assign(imageCount, VK_NULL_HANDLE);
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        const bool allocated = ctx.compositeDescriptorPool->allocateDescriptor(
            ctx.compositeSetLayout->getDescriptorSetLayout(),
            ctx.compositeDescriptorSets[i]);
        PIECE_CORE_ASSERT(allocated, "Failed to allocate composite descriptor set");

        VkDescriptorImageInfo worldPosRoughnessInfo{};
        worldPosRoughnessInfo.sampler = ctx.compositeSampler;
            worldPosRoughnessInfo.imageView = useMsaaComposite
                ? ctx.offscreenFrames[i].msaaWorldPosRoughnessImageView
                : ctx.offscreenFrames[i].worldPosRoughnessImageView;
        worldPosRoughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo albedoAoInfo{};
        albedoAoInfo.sampler = ctx.compositeSampler;
            albedoAoInfo.imageView = useMsaaComposite
                ? ctx.offscreenFrames[i].msaaAlbedoAoImageView
                : ctx.offscreenFrames[i].albedoAoImageView;
        albedoAoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo normalAoInfo{};
        normalAoInfo.sampler = ctx.compositeSampler;
            normalAoInfo.imageView = useMsaaComposite
                ? ctx.offscreenFrames[i].msaaNormalAoImageView
                : ctx.offscreenFrames[i].normalAoImageView;
        normalAoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo emissiveInfo{};
        emissiveInfo.sampler = ctx.compositeSampler;
            emissiveInfo.imageView = useMsaaComposite
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

        const bool presentAllocated = ctx.presentDescriptorPool->allocateDescriptor(
            ctx.presentSetLayout->getDescriptorSetLayout(),
            ctx.presentDescriptorSets[i]);
        PIECE_CORE_ASSERT(presentAllocated, "Failed to allocate present descriptor set");

        VkDescriptorImageInfo lightingColorInfo{};
        lightingColorInfo.sampler = ctx.compositeSampler;
        lightingColorInfo.imageView = ctx.offscreenFrames[i].lightingColorImageView;
        lightingColorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo presentBloomInfo{};
        presentBloomInfo.sampler = ctx.compositeSampler;
        presentBloomInfo.imageView = ctx.offscreenFrames[i].bloomExtractImageView;
        presentBloomInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        DescriptorWriter(*ctx.presentSetLayout, *ctx.presentDescriptorPool)
            .writeImage(0, &lightingColorInfo)
            .writeImage(1, &presentBloomInfo)
            .overwrite(ctx.presentDescriptorSets[i]);

        const bool extractAllocated = ctx.bloomDescriptorPool->allocateDescriptor(
            ctx.bloomSetLayout->getDescriptorSetLayout(), ctx.bloomExtractDescriptorSets[i]);
        const bool blurAllocated = ctx.bloomDescriptorPool->allocateDescriptor(
            ctx.bloomSetLayout->getDescriptorSetLayout(), ctx.bloomBlurDescriptorSets[i]);
        const bool verticalAllocated = ctx.bloomDescriptorPool->allocateDescriptor(
            ctx.bloomSetLayout->getDescriptorSetLayout(), ctx.bloomVerticalDescriptorSets[i]);
        PIECE_CORE_ASSERT(extractAllocated && blurAllocated && verticalAllocated, "Failed to allocate bloom descriptor sets");

        VkDescriptorImageInfo bloomInputInfo{};
        bloomInputInfo.sampler = ctx.compositeSampler;
        bloomInputInfo.imageView = ctx.offscreenFrames[i].lightingColorImageView;
        bloomInputInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo bloomParamsInfo{};
        bloomParamsInfo.sampler = ctx.compositeSampler;
        bloomParamsInfo.imageView = ctx.offscreenFrames[i].bloomParamsImageView;
        bloomParamsInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkDescriptorImageInfo emissiveBloomInfo{};
        emissiveBloomInfo.sampler = ctx.compositeSampler;
        emissiveBloomInfo.imageView = ctx.offscreenFrames[i].emissiveImageView;
        emissiveBloomInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        DescriptorWriter(*ctx.bloomSetLayout, *ctx.bloomDescriptorPool)
            .writeImage(0, &bloomInputInfo)
            .writeImage(1, &bloomParamsInfo)
            .writeImage(2, &emissiveBloomInfo)
            .overwrite(ctx.bloomExtractDescriptorSets[i]);

        VkDescriptorImageInfo bloomExtractInfo{};
        bloomExtractInfo.sampler = ctx.compositeSampler;
        bloomExtractInfo.imageView = ctx.offscreenFrames[i].bloomExtractImageView;
        bloomExtractInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        DescriptorWriter(*ctx.bloomSetLayout, *ctx.bloomDescriptorPool)
            .writeImage(0, &bloomExtractInfo)
            .writeImage(1, &bloomParamsInfo)
            .writeImage(2, &emissiveBloomInfo)
            .overwrite(ctx.bloomBlurDescriptorSets[i]);

        VkDescriptorImageInfo bloomBlurInfo{};
        bloomBlurInfo.sampler = ctx.compositeSampler;
        bloomBlurInfo.imageView = ctx.offscreenFrames[i].bloomBlurImageView;
        bloomBlurInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        DescriptorWriter(*ctx.bloomSetLayout, *ctx.bloomDescriptorPool)
            .writeImage(0, &bloomBlurInfo)
            .writeImage(1, &bloomParamsInfo)
            .writeImage(2, &emissiveBloomInfo)
            .overwrite(ctx.bloomVerticalDescriptorSets[i]);

    }
}

void CreateBillboardResources(RendererContext &ctx)
{
    if (!ctx.pointLightIconTexture)
    {
        const std::array<std::string, 2> candidates = {
            "PieceEditor/assets/icons/point_light.png",
            "assets/icons/point_light.png"};

        std::string iconPath = candidates[0];
        for (const std::string &candidate : candidates)
        {
            if (std::filesystem::exists(candidate))
            {
                iconPath = candidate;
                break;
            }
        }

        ctx.pointLightIconTexture = CreateRef<Texture>(*ctx.deviceWrapper, iconPath);
    }

    ctx.billboardSetLayout = DescriptorSetLayout::Builder(*ctx.deviceWrapper)
                                  .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                  .build();

    ctx.billboardDescriptorPool = DescriptorPool::Builder(*ctx.deviceWrapper)
                                       .setMaxSets(1)
                                       .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                                       .build();

    const bool allocated = ctx.billboardDescriptorPool->allocateDescriptor(
        ctx.billboardSetLayout->getDescriptorSetLayout(),
        ctx.pointLightIconDescriptorSet);
    PIECE_CORE_ASSERT(allocated, "Failed to allocate billboard icon descriptor set");

    VkDescriptorImageInfo iconInfo{};
    iconInfo.sampler = ctx.pointLightIconTexture->getSampler();
    iconInfo.imageView = ctx.pointLightIconTexture->getImageView();
    iconInfo.imageLayout = ctx.pointLightIconTexture->getImageLayout();

    DescriptorWriter(*ctx.billboardSetLayout, *ctx.billboardDescriptorPool)
        .writeImage(0, &iconInfo)
        .overwrite(ctx.pointLightIconDescriptorSet);
}

void CreateGraphicsPipeline(RendererContext& ctx) {
    const EnvironmentSettings environment = World::GetEnvironmentSettings();
    const bool useMsaaPath =
        environment.aaTechnique == AATechnique::MSAA &&
        ctx.msaaSamples != VK_SAMPLE_COUNT_1_BIT;

    PipelineConfigInfo geometryConfig{};
    Pipeline::defaultPipelineConfigInfo(geometryConfig);
    geometryConfig.renderPass = ctx.geometryRenderPass;
    geometryConfig.pipelineLayout = ctx.geometryPipelineLayout;
    geometryConfig.colorBlendAttachments = {
        geometryConfig.colorBlendAttachments[0],
        geometryConfig.colorBlendAttachments[0],
        geometryConfig.colorBlendAttachments[0],
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

    ctx.geometryPipeline = CreateScope<Pipeline>(
        *ctx.deviceWrapper,
        *ctx.shaderLibrary->Get("textured.vert"),
        *ctx.shaderLibrary->Get("textured.frag"),
        geometryConfig);

    PipelineConfigInfo billboardConfig{};
    Pipeline::defaultPipelineConfigInfo(billboardConfig);
    billboardConfig.renderPass = ctx.geometryRenderPass;
    billboardConfig.pipelineLayout = ctx.billboardPipelineLayout;
    billboardConfig.colorBlendAttachments = geometryConfig.colorBlendAttachments;
    billboardConfig.colorBlendInfo.attachmentCount = static_cast<uint32_t>(billboardConfig.colorBlendAttachments.size());
    billboardConfig.colorBlendInfo.pAttachments = billboardConfig.colorBlendAttachments.data();
    billboardConfig.multisampleInfo.rasterizationSamples = ctx.msaaSamples;
    billboardConfig.multisampleInfo.sampleShadingEnable = geometryConfig.multisampleInfo.sampleShadingEnable;
    billboardConfig.multisampleInfo.minSampleShading = geometryConfig.multisampleInfo.minSampleShading;

    ctx.billboardPipeline = CreateScope<Pipeline>(
        *ctx.deviceWrapper,
        *ctx.shaderLibrary->Get("billboard.vert"),
        *ctx.shaderLibrary->Get("billboard.frag"),
        billboardConfig);

    PipelineConfigInfo lightingConfig{};
    Pipeline::defaultPipelineConfigInfo(lightingConfig);
    lightingConfig.renderPass = ctx.lightingRenderPass;
    lightingConfig.pipelineLayout = ctx.lightingPipelineLayout;
    lightingConfig.bindingDescriptions.clear();
    lightingConfig.attributeDescriptions.clear();
    lightingConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
    lightingConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
    lightingConfig.multisampleInfo.rasterizationSamples = ctx.msaaSamples;

    const bool useLightingSampleRateShading =
        (ctx.msaaSamples != VK_SAMPLE_COUNT_1_BIT) &&
        ctx.deviceWrapper->isSampleRateShadingEnabled();
    lightingConfig.multisampleInfo.sampleShadingEnable = useLightingSampleRateShading ? VK_TRUE : VK_FALSE;
    lightingConfig.multisampleInfo.minSampleShading = useLightingSampleRateShading ? 1.0f : 0.0f;

    ctx.lightingPipeline = CreateScope<Pipeline>(
        *ctx.deviceWrapper,
        *ctx.shaderLibrary->Get("lighting_composite.vert"),
        *ctx.shaderLibrary->Get(
            useMsaaPath
                ? "lighting_composite_msaa.frag"
                : "lighting_composite.frag"),
        lightingConfig);

    PipelineConfigInfo bloomConfig{};
    Pipeline::defaultPipelineConfigInfo(bloomConfig);
    bloomConfig.renderPass = ctx.bloomRenderPass;
    bloomConfig.pipelineLayout = ctx.bloomPipelineLayout;
    bloomConfig.bindingDescriptions.clear();
    bloomConfig.attributeDescriptions.clear();
    bloomConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
    bloomConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
    bloomConfig.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    ctx.bloomExtractPipeline = CreateScope<Pipeline>(
        *ctx.deviceWrapper,
        *ctx.shaderLibrary->Get("lighting_composite.vert"),
        *ctx.shaderLibrary->Get("bloom_extract.frag"),
        bloomConfig);
    ctx.bloomBlurPipeline = CreateScope<Pipeline>(
        *ctx.deviceWrapper,
        *ctx.shaderLibrary->Get("lighting_composite.vert"),
        *ctx.shaderLibrary->Get("bloom_blur.frag"),
        bloomConfig);
    ctx.bloomVerticalPipeline = CreateScope<Pipeline>(
        *ctx.deviceWrapper,
        *ctx.shaderLibrary->Get("lighting_composite.vert"),
        *ctx.shaderLibrary->Get("bloom_blur_vertical.frag"),
        bloomConfig);

    PipelineConfigInfo presentConfig{};
    Pipeline::defaultPipelineConfigInfo(presentConfig);
    presentConfig.renderPass = ctx.presentRenderPass;
    presentConfig.pipelineLayout = ctx.presentPipelineLayout;
    presentConfig.bindingDescriptions.clear();
    presentConfig.attributeDescriptions.clear();
    presentConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
    presentConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

    ctx.presentPipeline = CreateScope<Pipeline>(
        *ctx.deviceWrapper,
        *ctx.shaderLibrary->Get("lighting_composite.vert"),
        *ctx.shaderLibrary->Get("present.frag"),
        presentConfig);
}

} // namespace RendererInternals

} // namespace Piece
