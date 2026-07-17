#include <PiecePCH.h>

#include "Scene.h"

#include "Components.h"
#include "Entity.h"

namespace Piece {

namespace {

template <typename Component>
void CopyComponent(entt::registry& destination, entt::registry& source, const std::unordered_map<UUID, entt::entity>& entityMap) {
    auto view = source.view<Component, TagComponent>();
    for (auto entity : view) {
        const UUID uuid = source.get<TagComponent>(entity).id;
        auto destinationEntity = entityMap.find(uuid);
        PIECE_CORE_ASSERT(destinationEntity != entityMap.end(), "Destination entity missing during scene copy");
        destination.emplace_or_replace<Component>(destinationEntity->second, source.get<Component>(entity));
    }
}

} // namespace

Ref<Scene> Scene::Copy(const Ref<Scene>& other) {
    Ref<Scene> newScene = CreateRef<Scene>();
    newScene->m_viewportWidth = other->m_viewportWidth;
    newScene->m_viewportHeight = other->m_viewportHeight;

    std::unordered_map<UUID, entt::entity> entityMap;
    auto view = other->m_registry.view<TagComponent>();
    for (auto entity : view) {
        const auto& tag = other->m_registry.get<TagComponent>(entity);
        Entity newEntity = newScene->CreateEntity(tag.tag, tag.id);
        entityMap[tag.id] = static_cast<entt::entity>(newEntity);
    }

    CopyComponent<TransformComponent>(newScene->m_registry, other->m_registry, entityMap);
    CopyComponent<MeshRendererComponent>(newScene->m_registry, other->m_registry, entityMap);
    CopyComponent<DirectionalLightComponent>(newScene->m_registry, other->m_registry, entityMap);
    CopyComponent<PointLightComponent>(newScene->m_registry, other->m_registry, entityMap);
    CopyComponent<SpotLightComponent>(newScene->m_registry, other->m_registry, entityMap);
    CopyComponent<CameraComponent>(newScene->m_registry, other->m_registry, entityMap);

    newScene->m_entityCount = other->m_entityCount;
    newScene->m_directionalLightCount = other->m_directionalLightCount;
    newScene->m_pointLightCount = other->m_pointLightCount;
    newScene->m_spotLightCount = other->m_spotLightCount;
    return newScene;
}

Entity Scene::CreateEntity(const std::string& name, UUID uuid) {
    Entity entity{m_registry.create(), this};
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name, uuid);
    entity.AddComponent<TransformComponent>();
    ++m_entityCount;
    return entity;
}

Entity Scene::CreateQuad(const std::string& name, UUID uuid) {
    Entity entity = CreateEntity(name, uuid);
    entity.AddComponent<MeshRendererComponent>(PrimitiveType::Quad);
    return entity;
}

Entity Scene::CreateCube(const std::string& name, UUID uuid) {
    Entity entity = CreateEntity(name, uuid);
    entity.AddComponent<MeshRendererComponent>(PrimitiveType::Cube);
    return entity;
}

Entity Scene::CreateSphere(const std::string& name, UUID uuid) {
    Entity entity = CreateEntity(name, uuid);
    entity.AddComponent<MeshRendererComponent>(PrimitiveType::Sphere);
    return entity;
}

Entity Scene::CreateDirectionalLight(UUID uuid) {
    Entity entity = CreateEntity("Directional Light " + std::to_string(m_directionalLightCount), uuid);
    entity.AddComponent<DirectionalLightComponent>();
    ++m_directionalLightCount;
    return entity;
}

Entity Scene::CreatePointLight(UUID uuid) {
    Entity entity = CreateEntity("Point Light " + std::to_string(m_pointLightCount), uuid);
    entity.AddComponent<PointLightComponent>();
    ++m_pointLightCount;
    return entity;
}

Entity Scene::CreateSpotLight(UUID uuid) {
    Entity entity = CreateEntity("Spot Light " + std::to_string(m_spotLightCount), uuid);
    entity.AddComponent<SpotLightComponent>();
    ++m_spotLightCount;
    return entity;
}

void Scene::DestroyEntity(Entity entity) {
    if (!entity) {
        return;
    }

    if (entity.HasComponent<DirectionalLightComponent>() && m_directionalLightCount > 0) {
        --m_directionalLightCount;
    }
    if (entity.HasComponent<PointLightComponent>() && m_pointLightCount > 0) {
        --m_pointLightCount;
    }
    if (entity.HasComponent<SpotLightComponent>() && m_spotLightCount > 0) {
        --m_spotLightCount;
    }
    if (m_entityCount > 0) {
        --m_entityCount;
    }

    m_registry.destroy(static_cast<entt::entity>(entity));
}

void Scene::Clear() {
    m_registry.clear();
    m_entityCount = 0;
    m_directionalLightCount = 0;
    m_pointLightCount = 0;
    m_spotLightCount = 0;
}

void Scene::OnViewportResize(uint32_t width, uint32_t height) {
    m_viewportWidth = width;
    m_viewportHeight = height;

    auto cameraView = m_registry.view<CameraComponent>();
    for (auto entity : cameraView) {
        auto& cameraComponent = cameraView.get<CameraComponent>(entity);
        if (!cameraComponent.fixedAspectRatio && width > 0 && height > 0) {
            cameraComponent.camera.setPerspective(70.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
            cameraComponent.camera.updateView();
        }
    }
}

Entity Scene::GetPrimaryCameraEntity() {
    auto cameraView = m_registry.view<CameraComponent>();
    for (auto entity : cameraView) {
        const auto& cameraComponent = cameraView.get<CameraComponent>(entity);
        if (cameraComponent.primary) {
            return Entity{entity, this};
        }
    }

    return {};
}

template <>
void Scene::OnComponentAdded<CameraComponent>(Entity, CameraComponent& component) {
    if (m_viewportWidth > 0 && m_viewportHeight > 0) {
        component.camera.setPerspective(70.0f, static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight), 0.1f, 100.0f);
        component.camera.updateView();
    }
}

} // namespace Piece
