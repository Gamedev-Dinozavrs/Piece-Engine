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

    VkSampler getSampler() const { return sampler_; }
    VkImageView getImageView() const { return imageView_; }
    VkImageLayout getImageLayout() const { return imageLayout_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    uint32_t getMipLevels() const { return mipLevels_; }

private:
    void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
    void generateMipmaps();

    Device& device_;
    VkImage image_{ VK_NULL_HANDLE };
    VmaAllocation imageAllocation_{ nullptr };
    VkImageView imageView_{ VK_NULL_HANDLE };
    VkSampler sampler_{ VK_NULL_HANDLE };
    VkFormat imageFormat_ = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageLayout imageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;

    int width_ = 0;
    int height_ = 0;
    uint32_t mipLevels_ = 1;
};

} // namespace Piece
