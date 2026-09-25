#include <PiecePCH.h>

#include "GameLayer.h"

#include <scene/Components.h>
#include <scene/Entity.h>
#include <scene/Scene.h>
#include <scene/World.h>
#include <scripting/ScriptComponent.h>

namespace Piece {

GameLayer::GameLayer() : Layer("GameLayer") {
}

void GameLayer::OnAttach() {
    // World::SpawnCube lazily creates the active scene (with a default Main Camera + light) if needed.
    const uint32_t cubeId = World::SpawnCube(SpawnTransform{});
    Ref<Scene> scene = World::GetActiveScene();

    Entity cube{static_cast<entt::entity>(cubeId), scene.get()};
    auto& script = cube.AddComponent<ScriptComponent>();
    script.className = "PieceEngine.Examples.RotateScript";

    scene->OnRuntimeStart();
}

void GameLayer::OnDetach() {
    if (Ref<Scene> scene = World::GetActiveScene()) {
        scene->OnRuntimeStop();
    }
}

void GameLayer::OnUpdate(Timestep ts) {
    if (Ref<Scene> scene = World::GetActiveScene()) {
        scene->OnUpdateRuntime(ts);
    }
}

} // namespace Piece
