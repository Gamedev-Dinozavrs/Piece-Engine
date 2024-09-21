//pch
#include <core/Application.h>
#include <window/PieceWindowGLFW.h>

namespace Piece {

	Application* Application::s_Instance = nullptr;

	Application::Application(const std::string& name) {

		PIECE_CORE_ASSERT(!s_instance, "Application already exist!");
		s_Instance = this;

		/*
		*  In future if we need more than 1 window framework we include it's implementation here and use it as we need!
		*/
		m_Window = CreateScope<PieceWindowGLFW>(WindowProperties(name));
		m_Window->SetEventCallback(PIECE_BIND_EVENT_FUNC(Application::OnEvent));

		// Renderer::Init();

		//imgui layer
		//pushoverlay imgui
	}

	Application::~Application() {}

	void Application::Run() {
		m_IsRunning = true;

		while (m_IsRunning) {

			float time = (float)glfwGetTime(); // platform dependent
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_IsMinimized) {
				for (Layer* layer : m_LayerStack) {
					layer->OnUpdate(timestep);
				}
				
				//m_imguiLayer->begin();
				for (Layer* layer : m_LayerStack) {
					//layer->onImGuiRender();
				}
				//m_imguiLayer->end();
			}

			m_Window->OnUpdate();
		}
	}

	void Application::OnEvent(Event& event) {
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(PIECE_BIND_EVENT_FUNC(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(PIECE_BIND_EVENT_FUNC(Application::OnWindowResize));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();) {
			if (event.IsHandled)
				break;
			(*--it)->OnEvent(event);
		}
	}

	void Application::PushLayer(Layer* layer) {
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay) {
		m_LayerStack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	bool Application::OnWindowClose(WindowCloseEvent& event) {
		m_IsRunning = false;
		return true;
	}
	
	bool Application::OnWindowResize(WindowResizeEvent& event) {
		if (event.getWidth() == 0 || event.getHeight() == 0) {
			m_IsMinimized = true;
			return true;
		}
		else
			m_IsMinimized = false;
		return false;
	}

} // namespace Piece