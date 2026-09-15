#pragma once

#include <renderer/Device.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <core/Core.h>

namespace Piece {

class Buffer {
public:
    Buffer(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    void map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void unmap();
    void write(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    void read(void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

    VkBuffer getBuffer() const { return buffer_; }
    VkDeviceSize size() const { return size_; }

    // Create a device-local buffer and upload data via a staging buffer.
    static Scope<Buffer> createDeviceLocal(Device& device, const void* data, VkDeviceSize size, VkBufferUsageFlags usage);

private:
    Device& device_;
    VkBuffer buffer_{ VK_NULL_HANDLE };
    VmaAllocation allocation_{ nullptr };
    void* mapped_ = nullptr;
    VkDeviceSize size_ = 0;
    VkMemoryPropertyFlags properties_ = 0;
};

} // namespace Piece
