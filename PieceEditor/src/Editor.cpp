//pch
//inc

#include "../../PieceLib/src/core/Application.h"
#include "EditorLayer.h"


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