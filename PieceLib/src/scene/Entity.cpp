#include <PiecePCH.h>
#include "Entity.h"

namespace Piece {

	Entity::Entity() :m_entityHandle{ entt::null }, m_scene(nullptr) {}
	Entity::Entity(entt::entity handle, Scene* scene) :m_entityHandle(handle), m_scene(scene) {}

} // namespace Piece