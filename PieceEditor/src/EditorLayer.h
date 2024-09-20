#pragma once

//#include <Piece.h>

#include <core/EntryPoint.h>
#include <layer/Layer.h>
#include <iostream>

namespace Piece {
	class EditorLayer : public Layer {
	public:
		EditorLayer() { std::cout << "Editor Layer Constructor called!\n"; }
		~EditorLayer() {}
	};

	/*
	class EditorLayer : public Layer {
	public:
		EditorLayer();
		virtual ~EditorLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& event) override;

		void ResizeFramebuffer();

		void OnScenePlay(); // being controlled by UI_Toolbar function
		void OnSceneStop(); // being controlled by UI_Toolbar function
		
		void UI_Toolbar();

	private:

		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);

		void CreateNewScene();
		void OpenScene();
		void OpenScene(const std::filesystem::path& filepath);
		void SaveSceneAs();

	private:

		// panel stuff

		// scene stuff

	};
	*/
} // namespace Piece