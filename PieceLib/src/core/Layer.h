#pragma once

#include <string>

#include "Timestep.h"

namespace Piece {

	class Layer {

	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void onAttach() {}
		virtual void onDetach() {}
		virtual void onUpdate(Timestep ts) {}
		virtual void onImGuiRender() {}
		//virtual void onEvent(Event& event) {}

		inline const std::string& getName() const { return m_debugName; }

	private:
		std::string m_debugName;
	};

} // namespace Piece