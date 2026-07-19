#pragma once

#include <renderer/Shader.h>
#include <core/Core.h>
#include <string>
#include <unordered_map>

namespace Piece {

class ShaderLibrary {
public:
    explicit ShaderLibrary(Device& device);

    // Load a shader and register it under name.
    Ref<Shader> Load(const std::string& name,
                      const std::string& filepath,
                      Shader::Stage stage);

    // Returns previously loaded shader. Throws if not found.
    Ref<Shader> Get(const std::string& name) const;

    bool Has(const std::string& name) const;

    // Destroy all cached shaders.
    void Clear();

private:
    Device& m_Device;
    std::unordered_map<std::string, Ref<Shader>> m_Shaders;
};

} // namespace Piece
