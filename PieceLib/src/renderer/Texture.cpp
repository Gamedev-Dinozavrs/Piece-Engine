#include <PiecePCH.h>

#include <stb_image.h>

#include "Texture.h"
#include "Buffer.h"

#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace Piece {

Texture::Texture(Device& device, const std::string& filepath)
    : m_Device(device) {
    int channels;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &m_Width, &m_Height, &channels, STBI_rgb_alpha);
    stbi_uc fallbackPixel[4] = { 255, 255, 255, 255 };
    if (!pixels) {
        m_Width = 1;
        m_Height = 1;
        channels = 4;
        pixels = fallbackPixel;
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(m_Width) * static_cast<VkDeviceSize>(m_Height) * 4;
    m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;

    // staging buffer
    auto staging = CreateScope<Buffer>(m_Device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    staging->map();
    staging->write(pixels, imageSize, 0);
    staging->flush(imageSize, 0);
    staging->unmap();

    m_ImageFormat = VK_FORMAT_R8G8B8A8_SRGB;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = m_ImageFormat;
    imageInfo.mipLevels = m_MipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.extent = { static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height), 1 };
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    m_Device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_ImageAllocation);

    transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    m_Device.copyBufferToImage(staging->getBuffer(), m_Image, static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height), 1);

    generateMipmaps();

    m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(m_MipLevels);
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    if (vkCreateSampler(m_Device.device(), &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_ImageFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_MipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_Device.device(), &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    if (pixels != fallbackPixel) {
        stbi_image_free(pixels);
    }
}

Texture::~Texture() {
    if (m_Sampler != VK_NULL_HANDLE) vkDestroySampler(m_Device.device(), m_Sampler, nullptr);
    if (m_ImageView != VK_NULL_HANDLE) vkDestroyImageView(m_Device.device(), m_ImageView, nullptr);
    if (m_Image != VK_NULL_HANDLE) {
        if (m_ImageAllocation != nullptr) {
            vmaDestroyImage(m_Device.allocator(), m_Image, m_ImageAllocation);
        }
        else {
            vkDestroyImage(m_Device.device(), m_Image, nullptr);
        }
    }
}

void Texture::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer cmd = m_Device.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_Image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_MipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_Device.endSingleTimeCommands(cmd);
}

void Texture::generateMipmaps() {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_Device.getPhysicalDevice(), m_ImageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer cmd = m_Device.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = m_Image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = m_Width;
    int32_t mipHeight = m_Height;

    for (uint32_t i = 1; i < m_MipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;

        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmd,
            m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = m_MipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_Device.endSingleTimeCommands(cmd);
}

} // namespace Piece
