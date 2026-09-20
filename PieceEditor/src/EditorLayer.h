#pragma once

#include "ContentBrowserPanel.h"
#include "SceneHierarchyPanel.h"

#include <event/KeyEvent.h>
#include <layer/Layer.h>
#include <renderer/Texture.h>
#include <scene/Components.h>

#include <string>
#include <vector>

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
    enum class SceneState {
        Edit,
        Play,
        Pause
    };

    struct TransformCommand {
        UUID entityId{0};
        TransformComponent before;
        TransformComponent after;
    };

    bool OnKeyPressed(KeyPressedEvent& event);
    void DrawTransformGizmo();
    void DrawSimulationToolbar();
    void DrawMenuBar();
    void StartPlay();
    void TogglePause();
    void StopPlay();
    void PushTransformCommand(const TransformCommand& command);
    void Undo();
    void Redo();
    bool ApplyTransformCommand(const TransformCommand& command, bool useAfter);
    void NewScene();
    void OpenScene();
    void SaveScene();
    void SaveSceneAs();

private:
    bool m_ShowMetrics = false;
    bool m_ReviewMode = false;
    int m_GizmoOperation = 0;
    bool m_GizmoWasUsing = false;
    UUID m_GizmoEntityId{0};
    TransformComponent m_GizmoStartTransform;
    SceneState m_SceneState = SceneState::Edit;
    Ref<Scene> m_EditorScene;
    Ref<Scene> m_RuntimeScene;
    Ref<Texture> m_PlayIcon;
    Ref<Texture> m_PauseIcon;
    Ref<Texture> m_StopIcon;
    VkDescriptorSet m_PlayIconDescriptor = VK_NULL_HANDLE;
    VkDescriptorSet m_PauseIconDescriptor = VK_NULL_HANDLE;
    VkDescriptorSet m_StopIconDescriptor = VK_NULL_HANDLE;
    std::vector<TransformCommand> m_TransformHistory;
    size_t m_TransformHistoryCursor = 0;
    std::string m_CurrentScenePath;
    ContentBrowserPanel m_ContentBrowserPanel;
    SceneHierarchyPanel m_SceneHierarchyPanel;
};

} // namespace Piece