#include <PiecePCH.h>
#include <layer/LayerStack.h> 

namespace Piece {

	LayerStack::LayerStack() {}
	LayerStack::~LayerStack() {
		Clear();
	}

	void LayerStack::Clear() {
		for (Layer* layer : m_Layers) {
			layer->OnDetach();
			delete layer;
		}
		m_Layers.clear();
		m_LayerCountIndex = 0;
		m_OverlayCountIndex = 0;
	}

	void LayerStack::PushLayer(Layer* layer) {
		m_Layers.emplace(m_Layers.begin() + m_LayerCountIndex, layer);
		m_LayerCountIndex++;
	}

	void LayerStack::PushOverlay(Layer* overlay) {
		m_Layers.emplace_back(overlay);
		m_OverlayCountIndex++;
	}

	void LayerStack::PopLayer(Layer* layer) {
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.begin() + m_LayerCountIndex) {
			m_Layers.erase(it);
			m_LayerCountIndex--;
		}
	}

	void LayerStack::PopOverlay(Layer* overlay) {
		auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (it != m_Layers.end()) {
			m_Layers.erase(it);
			m_OverlayCountIndex--;
		}
	}


} // namespace Piece