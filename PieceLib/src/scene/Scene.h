#pragma once

#include <core/Core.h>
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

    Entity CreateEntity(const std::string& name = "Entity", UUID uuid = UUID{});
    Entity CreateQuad(const std::string& name = "Quad", UUID uuid = UUID{});
    Entity CreateCube(const std::string& name = "Cube", UUID uuid = UUID{});
    Entity CreateSphere(const std::string& name = "Sphere", UUID uuid = UUID{});
    Entity CreateDirectionalLight(UUID uuid = UUID{});
    Entity CreatePointLight(UUID uuid = UUID{});
    Entity CreateSpotLight(UUID uuid = UUID{});
    void DestroyEntity(Entity entity);
    void Clear();
    void OnViewportResize(uint32_t width, uint32_t height);
    Entity GetPrimaryCameraEntity();

    template <typename... Components>
    auto GetAllEntitiesViewWith() { return m_registry.view<Components...>(); }

private:
    template <typename T>
    void OnComponentAdded(Entity entity, T& component) {}

    entt::registry m_registry;
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    uint32_t m_entityCount = 0;
    uint32_t m_directionalLightCount = 0;
    uint32_t m_pointLightCount = 0;
    uint32_t m_spotLightCount = 0;

    friend class Entity;
};

} // namespace Piece