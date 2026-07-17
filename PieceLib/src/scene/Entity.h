#pragma once

#include <core/UUID.h>

#include "Components.h"
#include "Scene.h"

#pragma warning(push, 0)
#include <entt.hpp>
#pragma warning(pop)

namespace Piece {

	class Entity {
	public:
		Entity();
		Entity(entt::entity handle, Scene* scene);
		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args) {
			PIECE_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			
			T& component = m_scene->m_registry.emplace<T>(m_entityHandle, std::forward<Args>(args)...);
			m_scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template <typename T>
		T& GetComponent() {
			PIECE_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_scene->m_registry.get<T>(m_entityHandle);
		}

		template <typename T>
		bool HasComponent() {
			return m_scene->m_registry.any_of<T>(m_entityHandle);
		}

		template <typename T>
		void RemoveComponent() {
			PIECE_CORE_ASSERT(HasComponent<T>(), "Entity doesn't have specified component already!");
			m_scene->m_registry.remove<T>(m_entityHandle);
		}

		UUID getUUID() { return GetComponent<TagComponent>().id; }
		const UUID getUUID() const { return m_scene->m_registry.get<TagComponent>(m_entityHandle).id; }

		//operators
		bool operator==(const Entity& other) const {
			return m_entityHandle == other.m_entityHandle && m_scene == other.m_scene;
		}

		bool operator!=(const Entity& other) const {
			return !(*this == other);
		}

		operator bool() const { return m_entityHandle != entt::null; }
		operator uint32_t() const { return (uint32_t)m_entityHandle; }
		operator entt::entity() const { return m_entityHandle; }

	private:
		entt::entity m_entityHandle{ entt::null };
		Scene* m_scene = nullptr;
	};

} // namespace Piece