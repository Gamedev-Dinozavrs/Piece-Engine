#include <PiecePCH.h>

#include "Shader.h"

#include <fstream>
#include <stdexcept>

namespace Piece {

Shader::Shader(Device& device, const std::string& filepath, Stage stage)
    : m_Device(device), m_Stage(stage), m_Filepath(filepath) {

    auto code = readFile(filepath);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(m_Device.device(), &createInfo, nullptr, &m_Module) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module: " + filepath);
    }
}

Shader::~Shader() {
    if (m_Module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_Device.device(), m_Module, nullptr);
        m_Module = VK_NULL_HANDLE;
    }
}

VkShaderStageFlagBits Shader::getVkStage() const {
    switch (m_Stage) {
        case Stage::Vertex:      return VK_SHADER_STAGE_VERTEX_BIT;
        case Stage::Fragment:    return VK_SHADER_STAGE_FRAGMENT_BIT;
        case Stage::Compute:     return VK_SHADER_STAGE_COMPUTE_BIT;
        case Stage::Geometry:    return VK_SHADER_STAGE_GEOMETRY_BIT;
        case Stage::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case Stage::TessEval:    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        default:
            PIECE_CORE_ASSERT(false, "Unknown shader stage");
            return VK_SHADER_STAGE_VERTEX_BIT;
    }
}

std::vector<char> Shader::readFile(const std::string& filepath) {
    std::ifstream file{filepath, std::ios::ate | std::ios::binary};
    if (!file.is_open()) {
        PIECE_CORE_ERROR("Failed to open shader file: {}", filepath);
        throw std::runtime_error("Failed to open shader file: " + filepath);
    }

    const size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

} // namespace Piece
