//pch
//inc

#include <core/Application.h>
#include <core/EntryPoint.h>
#include "EditorLayer.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Piece {

	class PieceEditor : public Application {
	public:
		PieceEditor() : Application("Piece") {
			PushLayer(new EditorLayer());
		}

		~PieceEditor() {}
	};

	Application* CreateApplication() {
		return new PieceEditor();
	}

} // namespace Piece