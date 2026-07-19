#include <PiecePCH.h>

#include "Buffer.h"

#include <stdexcept>
#include <cstring>

namespace Piece {

Buffer::Buffer(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    : device_(device), size_(size), properties_(properties) {
    device_.createBuffer(size_, usage, properties_, buffer_, allocation_);
}

Buffer::~Buffer() {
    if (mapped_) {
        unmap();
    }
    if (buffer_ != VK_NULL_HANDLE && allocation_ != nullptr) {
        vmaDestroyBuffer(device_.allocator(), buffer_, allocation_);
        buffer_ = VK_NULL_HANDLE;
        allocation_ = nullptr;
    }
}

void Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
    if (mapped_) return;
    if (allocation_ == nullptr) {
        throw std::runtime_error("Buffer allocation is not initialized for mapping.");
    }
    vmaMapMemory(device_.allocator(), allocation_, &mapped_);
}

void Buffer::unmap() {
    if (!mapped_) return;
    if (allocation_ == nullptr) {
        throw std::runtime_error("Buffer allocation is not initialized for unmapping.");
    }
    vmaUnmapMemory(device_.allocator(), allocation_);
    mapped_ = nullptr;
}

void Buffer::write(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (!mapped_) {
        map(size, offset);
    }
    std::memcpy(static_cast<char*>(mapped_) + offset, data, static_cast<size_t>(size));
}

VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
    if (properties_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) return VK_SUCCESS;

    if (allocation_ == nullptr) {
        throw std::runtime_error("Buffer allocation is not initialized for flush.");
    }

    return vmaFlushAllocation(device_.allocator(), allocation_, offset, size);
}

VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
    if (properties_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) return VK_SUCCESS;

    if (allocation_ == nullptr) {
        throw std::runtime_error("Buffer allocation is not initialized for invalidate.");
    }

    return vmaInvalidateAllocation(device_.allocator(), allocation_, offset, size);
}

VkDescriptorBufferInfo Buffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) const {
    VkDescriptorBufferInfo info{};
    info.buffer = buffer_;
    info.offset = offset;
    info.range = size;
    return info;
}

Scope<Buffer> Buffer::createDeviceLocal(Device& device, const void* data, VkDeviceSize size, VkBufferUsageFlags usage) {
    // staging buffer
    auto staging = CreateScope<Buffer>(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    staging->map();
    staging->write(data, size, 0);
    staging->flush(size, 0);
    staging->unmap();

    auto dst = CreateScope<Buffer>(device, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    device.copyBuffer(staging->getBuffer(), dst->getBuffer(), size);
    return dst;
}

} // namespace Piece
