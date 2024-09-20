#pragma once

#include <string>

#include <core/Core.h>
#include <event/Event.h>
#include <core/Timestep.h>

namespace Piece {

	class Layer {
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string& getName() const { return m_debugName; }

	private:
		std::string m_debugName;
	};

} // namespace Piece