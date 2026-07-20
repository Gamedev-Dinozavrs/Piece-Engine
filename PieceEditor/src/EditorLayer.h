#pragma once

#include "ContentBrowserPanel.h"
#include "SceneHierarchyPanel.h"

#include <event/KeyEvent.h>
#include <layer/Layer.h>

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

private:
    bool m_ShowMetrics = false;
    bool m_ReviewMode = false;
    ContentBrowserPanel m_ContentBrowserPanel;
    SceneHierarchyPanel m_SceneHierarchyPanel;
};

} // namespace Piece