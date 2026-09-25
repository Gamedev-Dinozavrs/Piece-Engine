#pragma once

#include <core/Timestep.h>

namespace Piece {

class Scene;

// Drives ScriptComponent lifecycle (create/update/destroy) through the native ScriptEngine.
class ScriptSystem {
public:
    static void OnRuntimeStart(Scene& scene);
    static void OnUpdate(Scene& scene, Timestep timestep);
    static void OnRuntimeStop(Scene& scene);
};

} // namespace Piece
