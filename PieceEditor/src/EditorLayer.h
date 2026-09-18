#pragma once

#include "ContentBrowserPanel.h"
#include "SceneHierarchyPanel.h"

#include <event/KeyEvent.h>
#include <layer/Layer.h>

#include <string>

namespace Piece {

class EditorLayer : public Layer {
public:
    EditorLayer();
    ~EditorLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Timestep ts) override;
    void OnImGuiRender() override;
    void OnEvent(Event& event) override;

private:
    bool OnKeyPressed(KeyPressedEvent& event);
    void DrawTransformGizmo();
    void DrawMenuBar();
    void NewScene();
    void OpenScene();
    void SaveScene();
    void SaveSceneAs();

private:
    bool m_ShowMetrics = false;
    bool m_ReviewMode = false;
    int m_GizmoOperation = 0;
    std::string m_CurrentScenePath;
    ContentBrowserPanel m_ContentBrowserPanel;
    SceneHierarchyPanel m_SceneHierarchyPanel;
};

} // namespace Piece