#pragma once

#include <core/Core.h>
#include <core/Timestep.h>
#include <core/UUID.h>

#include <entt.hpp>

#include <cstdint>
#include <string>

namespace Piece {

class Entity;
struct CameraComponent;
class EditorCamera;

class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    static Ref<Scene> Copy(const Ref<Scene>& other);
    void OnUpdateRuntime(Timestep timestep);

    Entity CreateEntity(const std::string& name = "Entity", UUID uuid = UUID{});
    void DestroyEntity(Entity entity);
    void Clear();
    void OnViewportResize(uint32_t width, uint32_t height);
    Entity GetPrimaryCameraEntity();
    bool IsEmpty() const { return m_entityCount == 0; }
    uint32_t GetEntityCount() const { return m_entityCount; }

    template <typename... Components>
    auto GetAllEntitiesViewWith() { return m_registry.view<Components...>(); }

private:
    template <typename T>
    void OnComponentAdded(Entity entity, T& component) {}

    entt::registry m_registry;
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    uint32_t m_entityCount = 0;

    friend class Entity;
};

template <>
void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component);

} // namespace Piece