#pragma once

#include <core/Core.h>
#include <layer/Layer.h>

#include <vector> // TODO: temp maybe

namespace Piece {

	class LayerStack {
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* layer);
		void PopLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopOverlay(Layer* overlay);
		void Clear();

		std::vector<Layer*>::iterator begin()	{ return m_Layers.begin(); }
		std::vector<Layer*>::iterator end()		{ return m_Layers.end(); }
	private:
		std::vector<Layer*> m_Layers;
		uint32_t m_LayerCountIndex = 0, m_OverlayCountIndex = 0;
	};

} // namespace Piece