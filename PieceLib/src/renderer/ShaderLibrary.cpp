#include <PiecePCH.h>

#include "ShaderLibrary.h"

#include <stdexcept>

namespace Piece {

ShaderLibrary::ShaderLibrary(Device& device)
    : m_Device(device) {}

Ref<Shader> ShaderLibrary::Load(const std::string& name,
                                  const std::string& filepath,
                                  Shader::Stage stage) {
    auto shader = CreateRef<Shader>(m_Device, filepath, stage);
    m_Shaders[name] = shader;
    return shader;
}

Ref<Shader> ShaderLibrary::Get(const std::string& name) const {
    auto it = m_Shaders.find(name);
    if (it == m_Shaders.end()) {
        throw std::runtime_error("ShaderLibrary: shader not found: " + name);
    }
    return it->second;
}

bool ShaderLibrary::Has(const std::string& name) const {
    return m_Shaders.find(name) != m_Shaders.end();
}

void ShaderLibrary::Clear() {
    m_Shaders.clear();
}

} // namespace Piece
