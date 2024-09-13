//pch
//inc

#include "../../PieceLib/src/core/Application.h" // TODO: Add include dir
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