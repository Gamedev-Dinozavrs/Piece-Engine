#pragma once

#include <cstdint>
#include <string>

namespace Piece {

struct ScriptComponent {
    std::string assemblyPath;
    std::string className;
    bool enabled = true;
    uint64_t managedHandle = 0;

    ScriptComponent() = default;
    ScriptComponent(const std::string& scriptAssemblyPath, const std::string& scriptClassName)
        : assemblyPath(scriptAssemblyPath), className(scriptClassName) {}
};

} // namespace Piece
