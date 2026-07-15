#include "Mesh.h"
#include <cstring>
#include <stdexcept>

namespace Piece {

Mesh::Mesh(Device& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : device_(device) {
    if (vertices.empty()) {
        throw std::runtime_error("Mesh vertices cannot be empty");
    }

    VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();

    // Create device-local vertex buffer and upload via staging
    vertexBuffer_ = Piece::Buffer::createDeviceLocal(device_, vertices.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    if (!indices.empty()) {
        indexBuffer_ = Piece::Buffer::createDeviceLocal(device_, indices.data(), indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        indexCount_ = static_cast<uint32_t>(indices.size());
    }
}

Mesh::~Mesh() {
    // Buffer destructor handles cleanup
}

void Mesh::bind(VkCommandBuffer commandBuffer) const {
    VkBuffer vertexBuffers[] = { vertexBuffer_ ? vertexBuffer_->getBuffer() : VK_NULL_HANDLE };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    if (indexBuffer_) {
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer_->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }
}

void Mesh::draw(VkCommandBuffer commandBuffer) const {
    if (indexBuffer_ != VK_NULL_HANDLE) {
        vkCmdDrawIndexed(commandBuffer, indexCount_, 1, 0, 0, 0);
    }
    else {
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }
}

} // namespace Piece
