#include <PiecePCH.h>

#include "ScriptGlue.h"

#include <core/Input.h>
#include <scene/Components.h>
#include <scene/Entity.h>
#include <scene/Scene.h>
#include <scene/World.h>

namespace Piece {

namespace {

Entity FindEntityByUUID(UUID uuid) {
    Ref<Scene> scene = World::GetActiveScene();
    if (!scene) {
        return {};
    }

    auto view = scene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (view.get<TagComponent>(handle).id == uuid) {
            return Entity{handle, scene.get()};
        }
    }
    return {};
}

} // namespace

namespace ScriptGlue {

void GetPosition(int64_t entityId, float* outX, float* outY, float* outZ) {
    Entity entity = FindEntityByUUID(UUID(static_cast<uint64_t>(entityId)));
    const glm::vec3 position = (entity && entity.HasComponent<TransformComponent>())
        ? entity.GetComponent<TransformComponent>().position
        : glm::vec3(0.0f);
    *outX = position.x;
    *outY = position.y;
    *outZ = position.z;
}

void SetPosition(int64_t entityId, float x, float y, float z) {
    Entity entity = FindEntityByUUID(UUID(static_cast<uint64_t>(entityId)));
    if (entity && entity.HasComponent<TransformComponent>()) {
        entity.GetComponent<TransformComponent>().position = glm::vec3(x, y, z);
    }
}

void GetRotation(int64_t entityId, float* outX, float* outY, float* outZ) {
    Entity entity = FindEntityByUUID(UUID(static_cast<uint64_t>(entityId)));
    const glm::vec3 rotation = (entity && entity.HasComponent<TransformComponent>())
        ? entity.GetComponent<TransformComponent>().rotation
        : glm::vec3(0.0f);
    *outX = rotation.x;
    *outY = rotation.y;
    *outZ = rotation.z;
}

void SetRotation(int64_t entityId, float x, float y, float z) {
    Entity entity = FindEntityByUUID(UUID(static_cast<uint64_t>(entityId)));
    if (entity && entity.HasComponent<TransformComponent>()) {
        entity.GetComponent<TransformComponent>().rotation = glm::vec3(x, y, z);
    }
}

void GetScale(int64_t entityId, float* outX, float* outY, float* outZ) {
    Entity entity = FindEntityByUUID(UUID(static_cast<uint64_t>(entityId)));
    const glm::vec3 scale = (entity && entity.HasComponent<TransformComponent>())
        ? entity.GetComponent<TransformComponent>().scale
        : glm::vec3(1.0f);
    *outX = scale.x;
    *outY = scale.y;
    *outZ = scale.z;
}

void SetScale(int64_t entityId, float x, float y, float z) {
    Entity entity = FindEntityByUUID(UUID(static_cast<uint64_t>(entityId)));
    if (entity && entity.HasComponent<TransformComponent>()) {
        entity.GetComponent<TransformComponent>().scale = glm::vec3(x, y, z);
    }
}

int32_t IsKeyPressed(int32_t keyCode) {
    return Input::IsKeyPressed(keyCode) ? 1 : 0;
}

} // namespace ScriptGlue
} // namespace Piece
