#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <renderer/Device.h>

namespace Piece {

class Texture {
public:
    Texture(Device& device, const std::string& filepath);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    VkSampler getSampler() const { return m_Sampler; }
    VkImageView getImageView() const { return m_ImageView; }
    VkImageLayout getImageLayout() const { return m_ImageLayout; }
    int getWidth() const { return m_Width; }
    int getHeight() const { return m_Height; }
    uint32_t getMipLevels() const { return m_MipLevels; }

private:
    void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
    void generateMipmaps();

    Device& m_Device;
    VkImage m_Image{ VK_NULL_HANDLE };
    VmaAllocation m_ImageAllocation{ nullptr };
    VkImageView m_ImageView{ VK_NULL_HANDLE };
    VkSampler m_Sampler{ VK_NULL_HANDLE };
    VkFormat m_ImageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageLayout m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    int m_Width = 0;
    int m_Height = 0;
    uint32_t m_MipLevels = 1;
};

} // namespace Piece
