#pragma once

#include <core/Core.h>
#include <core/Timestep.h>
#include <window/Window.h>
#include <event/ApplicationEvent.h>
#include <event/MouseEvent.h>
#include <layer/Layer.h>
#include <layer/LayerStack.h>

namespace Piece {
	class ImGuiLayer;

	class Application {
	public:
		Application(const std::string& name = "Piece Engine");
		virtual ~Application();

		void Run();
		void OnEvent(Event& event);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		static Application& Get() { return *s_Instance; }
		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		void Close() { m_IsRunning = false; }
		inline Window& GetWindow() { return *m_Window; }

	private:
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);
		bool OnMouseScrolled(MouseScrolledEvent& event);

	private:
		Scope<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer = nullptr;

		bool m_IsRunning = false;
		bool m_IsMinimized = false;

		LayerStack m_LayerStack;

		float m_LastFrameTime = 0.0f;
		static Application* s_Instance;
	};

	Application* CreateApplication();

} // namespace Piece