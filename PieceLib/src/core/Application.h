#pragma once

#include <string> // temp
#include <memory> // temp
#include "Layer.h"

namespace Piece {

	class Application {
	public:
		Application(const std::string& name = "Piece Engine");
		virtual ~Application();

		void Run();
		//void OnEvent(Event& event);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		static Application Get() { return *s_Instance; }

		// ImGuiLayer ...

		void Close() { m_IsRunning = false; }
		//inline Window& GetWindow() { return *m_Window; }

	private:
		//bool OnWindowClose(WindowCloseEvent& event);
		//bool OnWindowResize(WindowResizeEvent& event);

	private:
		//std::unique_ptr<Window> m_Window;
		//ImGuiLayer ...

		bool m_IsRunning = false;
		bool m_IsMinimized = false;

		// LayerStack...
		float m_LastFrameTime = 0.0f;
		static Application* s_Instance;
	};

	Application* CreateApplication();

} // namespace Piece