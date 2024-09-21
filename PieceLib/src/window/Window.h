#pragma once

#include <functional> // Temp
#include <event/Event.h>

#define GLFW_INCLUDE_VULKAN // TODO: delete then
#include <GLFW/glfw3.h>

#include <string> // TODO: remove when pch added if needed

namespace Piece {

	struct WindowProperties {
		WindowProperties(const std::string& name_ = "Piece", uint32_t width_ = 1600, uint32_t height_ = 900)
			: name(name_), width(width_), height(height_) {}

		std::string name;
		uint32_t width;
		uint32_t height;
	};

	class Window {
	public:
		using EventCallback = std::function<void(Event&)>;

		virtual ~Window() {}
		
		virtual void OnUpdate() = 0;
		
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetEventCallback(const EventCallback& callbackFunc) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSyncOnOrNot() const = 0;

		virtual void* GetNativeWindow() const = 0;
	};

} // namespace Piece
