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
    ctx.fxaaDescriptorSets.clear();
    ctx.fxaaDescriptorPool.reset();
    ctx.taaDescriptorSets.clear();
    ctx.taaDescriptorPool.reset();
    ctx.fxaaSetLayout.reset();

    if (ctx.compositeSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(ctx.device, ctx.compositeSampler, nullptr);
        ctx.compositeSampler = VK_NULL_HANDLE;
    }

    if (ctx.taaHistoryImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(ctx.device, ctx.taaHistoryImageView, nullptr);
        ctx.taaHistoryImageView = VK_NULL_HANDLE;
    }
    if (ctx.taaHistoryImage != VK_NULL_HANDLE && ctx.taaHistoryAllocation != nullptr)
    {
        vmaDestroyImage(ctx.deviceWrapper->allocator(), ctx.taaHistoryImage, ctx.taaHistoryAllocation);
        ctx.taaHistoryImage = VK_NULL_HANDLE;
        ctx.taaHistoryAllocation = nullptr;
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
    if (ctx.fxaaPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(ctx.device, ctx.fxaaPipelineLayout, nullptr);
        ctx.fxaaPipelineLayout = VK_NULL_HANDLE;
    }
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
        depthAttachmentRef.attachment = 4;
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

        std::array<VkAttachmentDescription, 5> attachments = {
            worldPosRoughnessAttachment,
            albedoAoAttachment,
            normalAoAttachment,
            emissiveAttachment,
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

    std::array<VkAttachmentReference, 4> colorAttachmentRefs{};
    colorAttachmentRefs[0].attachment = 0;
    colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentRefs[1].attachment = 1;
    colorAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentRefs[2].attachment = 2;
    colorAttachmentRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentRefs[3].attachment = 3;
    colorAttachmentRefs[3].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 4> resolveAttachmentRefs{};
    resolveAttachmentRefs[0].attachment = 5;
    resolveAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolveAttachmentRefs[1].attachment = 6;
    resolveAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolveAttachmentRefs[2].attachment = 7;
    resolveAttachmentRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolveAttachmentRefs[3].attachment = 8;
    resolveAttachmentRefs[3].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 4;
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

    std::array<VkAttachmentDescription, 9> attachments = {
        worldPosRoughnessAttachment,
        albedoAoAttachment,
        normalAoAttachment,
        emissiveAttachment,
        depthAttachment,
        worldPosRoughnessAttachmentResolve,
        albedoAoAttachmentResolve,
        normalAoAttachmentResolve,
        emissiveAttachmentResolve
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

        std::array<VkImageView, 9> attachments{};
        uint32_t attachmentCount = 0;
        if (ctx.msaaSamples == VK_SAMPLE_COUNT_1_BIT)
        {
            attachments = {
                frame.worldPosRoughnessImageView,
                frame.albedoAoImageView,
                frame.normalAoImageView,
                frame.emissiveImageView,
                frame.depthImageView,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE};
            attachmentCount = 5;
        }
        else
        {
            attachments = {
                frame.msaaWorldPosRoughnessImageView,
                frame.msaaAlbedoAoImageView,
                frame.msaaNormalAoImageView,
                frame.msaaEmissiveImageView,
                frame.depthImageView,
                frame.worldPosRoughnessImageView,
                frame.albedoAoImageView,
                frame.normalAoImageView,
                frame.emissiveImageView};
            attachmentCount = 9;
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

    ctx.fxaaSetLayout = DescriptorSetLayout::Builder(*ctx.deviceWrapper)
                            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                            .build();

    ctx.fxaaDescriptorPool = DescriptorPool::Builder(*ctx.deviceWrapper)
                                 .setMaxSets(imageCount)
                                 .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount * 2)
                                 .build();

    ctx.taaDescriptorPool = DescriptorPool::Builder(*ctx.deviceWrapper)
                                 .setMaxSets(imageCount)
                                 .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount * 2)
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

    if (ctx.taaHistoryImage == VK_NULL_HANDLE)
    {
        VkImageCreateInfo historyImageInfo{};
        historyImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        historyImageInfo.imageType = VK_IMAGE_TYPE_2D;
        historyImageInfo.extent.width = ctx.swapChainExtent.width;
        historyImageInfo.extent.height = ctx.swapChainExtent.height;
        historyImageInfo.extent.depth = 1;
        historyImageInfo.mipLevels = 1;
        historyImageInfo.arrayLayers = 1;
        historyImageInfo.format = ctx.swapChainImageFormat;
        historyImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        historyImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        historyImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        historyImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        historyImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        ctx.deviceWrapper->createImageWithInfo(
            historyImageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            ctx.taaHistoryImage,
            ctx.taaHistoryAllocation);

        VkImageViewCreateInfo historyViewInfo{};
        historyViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        historyViewInfo.image = ctx.taaHistoryImage;
        historyViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        historyViewInfo.format = ctx.swapChainImageFormat;
        historyViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        historyViewInfo.subresourceRange.baseMipLevel = 0;
        historyViewInfo.subresourceRange.levelCount = 1;
        historyViewInfo.subresourceRange.baseArrayLayer = 0;
        historyViewInfo.subresourceRange.layerCount = 1;

        VkResult viewResult = vkCreateImageView(ctx.device, &historyViewInfo, nullptr, &ctx.taaHistoryImageView);
        PIECE_CORE_ASSERT(viewResult == VK_SUCCESS, "Failed to create TAA history image view");

        VkCommandBuffer initCommandBuffer = ctx.deviceWrapper->beginSingleTimeCommands();
        VkImageMemoryBarrier historyInitBarrier{};
        historyInitBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        historyInitBarrier.srcAccessMask = 0;
        historyInitBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        historyInitBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        historyInitBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        historyInitBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        historyInitBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        historyInitBarrier.image = ctx.taaHistoryImage;
        historyInitBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        historyInitBarrier.subresourceRange.baseMipLevel = 0;
        historyInitBarrier.subresourceRange.levelCount = 1;
        historyInitBarrier.subresourceRange.baseArrayLayer = 0;
        historyInitBarrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(
            initCommandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &historyInitBarrier);
        ctx.deviceWrapper->endSingleTimeCommands(initCommandBuffer);
    }

    ctx.compositeDescriptorSets.assign(imageCount, VK_NULL_HANDLE);
    ctx.fxaaDescriptorSets.assign(imageCount, VK_NULL_HANDLE);
    ctx.taaDescriptorSets.assign(imageCount, VK_NULL_HANDLE);
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

        const bool fxaaAllocated = ctx.fxaaDescriptorPool->allocateDescriptor(
            ctx.fxaaSetLayout->getDescriptorSetLayout(),
            ctx.fxaaDescriptorSets[i]);
        PIECE_CORE_ASSERT(fxaaAllocated, "Failed to allocate FXAA descriptor set");

        VkDescriptorImageInfo lightingColorInfo{};
        lightingColorInfo.sampler = ctx.compositeSampler;
        lightingColorInfo.imageView = ctx.offscreenFrames[i].lightingColorImageView;
        lightingColorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo worldPosRoughnessFxaaInfo{};
        worldPosRoughnessFxaaInfo.sampler = ctx.compositeSampler;
        worldPosRoughnessFxaaInfo.imageView = ctx.offscreenFrames[i].worldPosRoughnessImageView;
        worldPosRoughnessFxaaInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        DescriptorWriter(*ctx.fxaaSetLayout, *ctx.fxaaDescriptorPool)
            .writeImage(0, &lightingColorInfo)
            .writeImage(1, &worldPosRoughnessFxaaInfo)
            .overwrite(ctx.fxaaDescriptorSets[i]);

        const bool taaAllocated = ctx.taaDescriptorPool->allocateDescriptor(
            ctx.fxaaSetLayout->getDescriptorSetLayout(),
            ctx.taaDescriptorSets[i]);
        PIECE_CORE_ASSERT(taaAllocated, "Failed to allocate TAA descriptor set");

        VkDescriptorImageInfo taaHistoryInfo{};
        taaHistoryInfo.sampler = ctx.compositeSampler;
        taaHistoryInfo.imageView = ctx.taaHistoryImageView;
        taaHistoryInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        DescriptorWriter(*ctx.fxaaSetLayout, *ctx.taaDescriptorPool)
            .writeImage(0, &lightingColorInfo)
            .writeImage(1, &taaHistoryInfo)
            .overwrite(ctx.taaDescriptorSets[i]);
    }
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

    PipelineConfigInfo fxaaConfig{};
    Pipeline::defaultPipelineConfigInfo(fxaaConfig);
    fxaaConfig.renderPass = ctx.presentRenderPass;
    fxaaConfig.pipelineLayout = ctx.fxaaPipelineLayout;
    fxaaConfig.bindingDescriptions.clear();
    fxaaConfig.attributeDescriptions.clear();
    fxaaConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
    fxaaConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

    ctx.fxaaPipeline = CreateScope<Pipeline>(
        *ctx.deviceWrapper,
        *ctx.shaderLibrary->Get("lighting_composite.vert"),
        *ctx.shaderLibrary->Get("fxaa.frag"),
        fxaaConfig);

    ctx.taaPipeline = CreateScope<Pipeline>(
        *ctx.deviceWrapper,
        *ctx.shaderLibrary->Get("lighting_composite.vert"),
        *ctx.shaderLibrary->Get("taa.frag"),
        fxaaConfig);

    ctx.presentPipeline = CreateScope<Pipeline>(
        *ctx.deviceWrapper,
        *ctx.shaderLibrary->Get("lighting_composite.vert"),
        *ctx.shaderLibrary->Get("present.frag"),
        fxaaConfig);
}

} // namespace RendererInternals

} // namespace Piece
