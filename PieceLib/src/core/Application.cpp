//pch
#include "Application.h"
#include "Layer.h"

#define BIND_EVENT_FUNC(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }

namespace Piece {

	Application* Application::s_Instance = nullptr;

	Application::Application(const std::string& name) {

		//CORE_ASSERT(!s_instance, "Application already exist!");
		s_Instance = this;

		// m_Window = std::make_unique<Window>(Window::Create(WindowProps(name)));
		// m_Window->SetEventCallback(BIND_EVENT_FUNC(Application::OnEvent));

		// Renderer::Init();

		// imgui layer
		// pushoverlay imgui
	}

	Application::~Application() {}

	/*
	void Application::OnEvent(Event& event) {

		//EventDispatcher
		//disp
		//disp

		//
		// layer stack events
		//
	}

	void Application::pushLayer(Layer* layer) {
		RCKT_PROFILE_FUNCTION();

		m_layerStack.pushLayer(layer);
		layer->onAttach();
	}

	void Application::pushOverlay(Layer* overlay) {
		RCKT_PROFILE_FUNCTION();

		m_layerStack.pushOverlay(overlay);
		overlay->onAttach();
	}
	*/

	void Application::Run() {
		m_IsRunning = true;

		while (m_IsRunning) {

			//float time = (float)glfwGetTime(); // pLatform dependent get_time()
			//Timestep timestep = time - m_LastFrameTime;
			//m_LastFrameTime = time;

			if (!m_IsMinimized) {
				{
					/*
					for (Layer* layer : m_layerStack) {
						layer->onUpdate(timestep);
					}
					*/
				}
				
				/*
				m_imguiLayer->begin();
				
				{
					RCKT_PROFILE_SCOPE("ImguiLayerStack onImguiRender:");
					for (Layer* layer : m_layerStack) {
						layer->onImGuiRender();
					}
					m_imguiLayer->end();
				}
				*/
			}

			//m_window->onUpdate();
		}
	}
	void Application::PushLayer(Layer* layer)
	{
		// DOES NOTHING
	}
	void Application::PushOverlay(Layer* overlayer)
	{
		// DOES NOTHING
	}
	/*
	bool Application::onWindowClose(WindowCloseEvent& event) {
		m_running = false;
		return true;
	}

	bool Application::onWindowResize(WindowResizeEvent& event) {
		RCKT_PROFILE_FUNCTION();

		if (event.getWidth() == 0 || event.getHeight() == 0) {
			m_minimized = true;
			return false;
		}
		else
			m_minimized = false;

		//Renderer::onWindowResize(); no need to resize camera bounds
		return false;
	}


	*/

} // namespace Piece