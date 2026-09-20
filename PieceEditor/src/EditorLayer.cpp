#include "EditorLayer.h"

#include "imgui.h"
#include "imgui_internal.h"
#include <ImGuizmo.h>
#include <core/Input.h>
#include <core/KeyCodes.h>
#include <core/Log.h>
#include <event/Event.h>
#include <renderer/Renderer.h>
#include <scene/Components.h>
#include <scene/EditorCamera.h>
#include <scene/Scene.h>
#include <scene/SceneSerializer.h>
#include <scene/World.h>
#include <utils/platform/WindowsUtils.h>

namespace Piece {

EditorLayer::EditorLayer()
    : Layer("EditorLayer") {
}

EditorLayer::~EditorLayer() {
}

void EditorLayer::OnAttach() {
    m_SceneHierarchyPanel.SetContext(World::GetActiveScene());
}

void EditorLayer::OnDetach() {
}

void EditorLayer::OnUpdate(Timestep ts) {
    (void)ts;
}

void EditorLayer::OnEvent(Event& event) {
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<KeyPressedEvent>(PIECE_BIND_EVENT_FUNC(EditorLayer::OnKeyPressed));
}

bool EditorLayer::OnKeyPressed(KeyPressedEvent& event) {
    if (event.getRepeatCount() > 0) {
        return false;
    }

    const KeyCode key = ToKeyCode(event.getKeyCode());
    const bool altPressed = Input::IsKeyPressed(KeyCode::LeftAlt) || Input::IsKeyPressed(KeyCode::RightAlt);

    if (altPressed && key == KeyCode::Enter) {
        m_ReviewMode = !m_ReviewMode;
        return true;
    }

    if (m_ReviewMode && key == KeyCode::Escape) {
        m_ReviewMode = false;
        return true;
    }

    return false;
}

void EditorLayer::OnImGuiRender() {
    if (m_ReviewMode) {
        return;
    }

    m_SceneHierarchyPanel.SetContext(World::GetActiveScene());

    static bool dockspaceOpen = true;
    static bool fullscreen = true;
    static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    if (fullscreen) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) {
        windowFlags |= ImGuiWindowFlags_NoBackground;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Piece Editor", &dockspaceOpen, windowFlags);
    ImGui::PopStyleVar();

    if (fullscreen) {
        ImGui::PopStyleVar(2);
    }

    ImGuiIO& io = ImGui::GetIO();
    bool overEditorPanel = false;
    const char* editorPanelNames[] = {"Hierarchy", "Properties", "Content Browser"};
    for (const char* panelName : editorPanelNames) {
        ImGuiWindow* panel = ImGui::FindWindowByName(panelName);
        if (panel && ImGui::IsMouseHoveringRect(panel->Pos, ImVec2(panel->Pos.x + panel->Size.x, panel->Pos.y + panel->Size.y), true)) {
            overEditorPanel = true;
            break;
        }
    }

    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspaceId = ImGui::GetID("PieceDockspace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
    }

    DrawTransformGizmo();

    const bool leftClickInRenderArea = io.MouseClicked[ImGuiMouseButton_Left]
        && !overEditorPanel
        && !ImGui::IsAnyItemHovered();
    if (leftClickInRenderArea && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing()) {
        const ImVec2 mouse = io.MousePos;
        if (mouse.x >= 0.0f && mouse.y >= 0.0f) {
            const UUID picked = Renderer::ReadEntityIdAtPixel(
                static_cast<uint32_t>(mouse.x),
                static_cast<uint32_t>(mouse.y));
            m_SceneHierarchyPanel.SelectEntityByUUID(picked);
        }
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                NewScene();
            }
            if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
                OpenScene();
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                SaveScene();
            }
            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
                SaveSceneAs();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close Scene")) {
                World::ClearScene();
                m_CurrentScenePath.clear();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Metrics", nullptr, &m_ShowMetrics);
            const bool reviewModePreview = m_ReviewMode;
            if (ImGui::MenuItem("Review Mode (Alt+Enter)", nullptr, reviewModePreview)) {
                m_ReviewMode = !m_ReviewMode;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();

    m_SceneHierarchyPanel.OnImGuiRender();
    m_ContentBrowserPanel.SetAnimationTarget(
        m_SceneHierarchyPanel.GetSelectedEntity()
            ? static_cast<uint32_t>(m_SceneHierarchyPanel.GetSelectedEntity())
            : 0);
    m_ContentBrowserPanel.OnImGuiRender();

    Renderer::GetEditorCamera().setInputEnabled(!overEditorPanel);

    if (m_ShowMetrics) {
        ImGui::ShowMetricsWindow(&m_ShowMetrics);
    }
}

void EditorLayer::DrawTransformGizmo() {
    Entity selected = m_SceneHierarchyPanel.GetSelectedEntity();
    if (!selected || !selected.HasComponent<TransformComponent>()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 displaySize = io.DisplaySize;
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    ImGuizmo::SetRect(0.0f, 0.0f, displaySize.x, displaySize.y);
    ImGuizmo::SetOrthographic(false);

    if (ImGui::IsKeyPressed(ImGuiKey_G)) {
        m_GizmoOperation = (m_GizmoOperation + 1) % 3;
    }

    glm::mat4 transform = selected.GetComponent<TransformComponent>().GetTransform();
    const EditorCamera& camera = Renderer::GetEditorCamera();
    glm::mat4 gizmoProjection = camera.projection();
    gizmoProjection[1][1] *= -1.0f;
    const ImGuizmo::OPERATION operations[] = {
        ImGuizmo::TRANSLATE,
        ImGuizmo::ROTATE,
        ImGuizmo::SCALE};

    if (ImGuizmo::Manipulate(
            &camera.view()[0][0],
            &gizmoProjection[0][0],
            operations[m_GizmoOperation],
            ImGuizmo::LOCAL,
            &transform[0][0])) {
        float translation[3]{};
        float rotation[3]{};
        float scale[3]{};
        ImGuizmo::DecomposeMatrixToComponents(&transform[0][0], translation, rotation, scale);

        auto& entityTransform = selected.GetComponent<TransformComponent>();
        entityTransform.position = glm::vec3(translation[0], translation[1], translation[2]);
        entityTransform.rotation = glm::vec3(rotation[0], rotation[1], rotation[2]);
        entityTransform.scale = glm::vec3(scale[0], scale[1], scale[2]);
    }
}

namespace {
constexpr const char* kSceneFileFilter = "Piece Scene\0*.piecescene\0All Files\0*.*\0";
constexpr const char* kSceneFileExtension = "piecescene";
} // namespace

void EditorLayer::NewScene() {
    Renderer::WaitIdle();
    World::ClearScene();
    m_CurrentScenePath.clear();
    m_SceneHierarchyPanel.SetContext(World::GetActiveScene());
}

void EditorLayer::OpenScene() {
    const std::string path = Platform::OpenFileDialog(kSceneFileFilter);
    if (path.empty()) {
        return;
    }

    Renderer::WaitIdle();
    Ref<Scene> scene = CreateRef<Scene>();
    SceneSerializer serializer(scene);
    if (!serializer.Deserialize(path)) {
        PIECE_ERROR("Failed to open scene: {}", path);
        return;
    }

    m_CurrentScenePath = path;
    m_SceneHierarchyPanel.SetContext(World::GetActiveScene());
}

void EditorLayer::SaveScene() {
    if (m_CurrentScenePath.empty()) {
        SaveSceneAs();
        return;
    }

    Ref<Scene> scene = World::GetActiveScene();
    if (!scene) {
        return;
    }

    SceneSerializer serializer(scene);
    serializer.Serialize(m_CurrentScenePath);
}

void EditorLayer::SaveSceneAs() {
    Ref<Scene> scene = World::GetActiveScene();
    if (!scene) {
        return;
    }

    const std::string path = Platform::SaveFileDialog(kSceneFileFilter, kSceneFileExtension);
    if (path.empty()) {
        return;
    }

    SceneSerializer serializer(scene);
    serializer.Serialize(path);
    m_CurrentScenePath = path;
}

} // namespace Piece

