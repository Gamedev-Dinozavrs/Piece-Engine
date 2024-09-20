#pragma once

#include <window/Window.h>
#include <event/Event.h>

namespace Piece {

	class PieceWindowGLFW : public Window {
	public:
		PieceWindowGLFW(const WindowProperties& props);
		virtual ~PieceWindowGLFW();

		void OnUpdate() override;

		inline uint32_t GetWidth() const override { return m_Data.Width; }
		inline uint32_t GetHeight() const override { return m_Data.Height; }
		VkExtent2D GetExtent() const { return { static_cast<uint32_t>(m_Data.Width), static_cast<uint32_t>(m_Data.Height) }; }

		inline void SetEventCallback(const EventCallback& callback) override { m_Data.CallbackFunc = callback; }
		
		inline virtual void* GetNativeWindow() const override { return m_Window; }

		void SetVSync(bool enabled) override;
		bool IsVSyncOnOrNot() const override;

	private:
		virtual void Init(const WindowProperties& props);
		virtual void Shutdown();

		GLFWwindow* m_Window;
		struct WindowData {
			std::string Name;
			uint32_t Width = 1280, Height = 700;
			bool IsVSyncEnabled = false;
			
			EventCallback CallbackFunc;
		};

		WindowData m_Data;
	};

} // namespace Piece