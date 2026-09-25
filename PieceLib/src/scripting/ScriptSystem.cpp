#include <PiecePCH.h>

#include "ScriptSystem.h"

#include <core/Application.h>
#include <scene/Components.h>
#include <scene/Scene.h>
#include <scripting/ScriptComponent.h>
#include <scripting/ScriptEngine.h>

namespace Piece {

void ScriptSystem::OnRuntimeStart(Scene& scene) {
    ScriptEngine& engine = Application::Get().GetScriptEngine();
    if (!engine.IsInitialized()) {
        return;
    }

    auto view = scene.GetAllEntitiesViewWith<ScriptComponent, TagComponent>();
    for (auto handle : view) {
        const auto& script = view.get<ScriptComponent>(handle);
        if (!script.enabled || script.className.empty()) {
            continue;
        }

        const auto& tag = view.get<TagComponent>(handle);
        engine.CreateEntityScript(static_cast<uint64_t>(tag.id), script.className);
    }
}

void ScriptSystem::OnUpdate(Scene& scene, Timestep timestep) {
    ScriptEngine& engine = Application::Get().GetScriptEngine();
    if (!engine.IsInitialized()) {
        return;
    }

    auto view = scene.GetAllEntitiesViewWith<ScriptComponent, TagComponent>();
    for (auto handle : view) {
        const auto& script = view.get<ScriptComponent>(handle);
        if (!script.enabled || script.className.empty()) {
            continue;
        }

        const auto& tag = view.get<TagComponent>(handle);
        engine.UpdateEntityScript(static_cast<uint64_t>(tag.id), static_cast<float>(timestep));
    }
}

void ScriptSystem::OnRuntimeStop(Scene& scene) {
    ScriptEngine& engine = Application::Get().GetScriptEngine();
    if (!engine.IsInitialized()) {
        return;
    }

    auto view = scene.GetAllEntitiesViewWith<ScriptComponent, TagComponent>();
    for (auto handle : view) {
        const auto& script = view.get<ScriptComponent>(handle);
        if (!script.enabled || script.className.empty()) {
            continue;
        }

        const auto& tag = view.get<TagComponent>(handle);
        engine.DestroyEntityScript(static_cast<uint64_t>(tag.id));
    }
}

} // namespace Piece
