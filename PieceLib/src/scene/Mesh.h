#pragma once

#include <renderer/Device.h>
#include <renderer/Buffer.h>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace Piece {

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 uv;
    glm::vec3 normal;
};

class Mesh {
public:
    Mesh(Device& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~Mesh();

    void bind(VkCommandBuffer commandBuffer) const;
    void draw(VkCommandBuffer commandBuffer) const;

private:
    Device& device_;
    Scope<Buffer> vertexBuffer_;
    Scope<Buffer> indexBuffer_;
    uint32_t indexCount_{0};
};

} // namespace Piece
