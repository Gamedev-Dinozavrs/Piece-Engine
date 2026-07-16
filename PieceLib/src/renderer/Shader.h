#pragma once

#include <renderer/Device.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace Piece {

class Shader {
public:
    enum class Stage {
        Vertex,
        Fragment,
        Compute,
        Geometry,
        TessControl,
        TessEval
    };

    Shader(Device& device, const std::string& filepath, Stage stage);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    VkShaderModule       getModule()  const { return m_Module; }
    VkShaderStageFlagBits getVkStage() const;
    Stage                getStage()   const { return m_Stage; }
    const std::string&   getFilepath() const { return m_Filepath; }

private:
    static std::vector<char> readFile(const std::string& filepath);

    Device&        m_Device;
    VkShaderModule m_Module{VK_NULL_HANDLE};
    Stage          m_Stage;
    std::string    m_Filepath;
};

} // namespace Piece
