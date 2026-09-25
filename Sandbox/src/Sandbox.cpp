#include <core/Application.h>
#include <core/EntryPoint.h>
#include "GameLayer.h"

namespace Piece {

	class Sandbox : public Application {
	public:
		Sandbox() : Application("Sandbox") {
			PushLayer(new GameLayer());
		}

		~Sandbox() {}
	};

	Application* CreateApplication() {
		return new Sandbox();
	}

} // namespace Piece
