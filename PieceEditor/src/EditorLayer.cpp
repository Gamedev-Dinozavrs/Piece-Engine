#include "EditorLayer.h"

#include "imgui.h"
#include "EditorPlacement.h"
#include <ImGuizmo.h>
#include <core/Input.h>
#include <core/KeyCodes.h>
#include <event/Event.h>
#include <renderer/Renderer.h>
#include <scene/Components.h>
#include <scene/EditorCamera.h>
#include <scene/World.h>

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
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspaceId = ImGui::GetID("PieceDockspace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
    }

    DrawTransformGizmo();

    if (io.MouseClicked[0] && !io.WantCaptureMouse && !ImGui::IsAnyItemActive()
        && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing()) {
        const ImVec2 mouse = io.MousePos;
        if (mouse.x >= 0.0f && mouse.y >= 0.0f) {
            const UUID picked = Renderer::ReadEntityIdAtPixel(
                static_cast<uint32_t>(mouse.x),
                static_cast<uint32_t>(mouse.y));
            m_SceneHierarchyPanel.SelectEntityByUUID(picked);
        }
    }

    if (ImGui::BeginMenuBar()) {
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

    if (ImGui::BeginPopupContextWindow("RenderAreaContext", ImGuiPopupFlags_NoOpenOverItems)) {
        EditorPlacement::DrawCreateMenu();
        ImGui::EndPopup();
    }

    ImGui::End();

    m_SceneHierarchyPanel.OnImGuiRender();
    m_ContentBrowserPanel.OnImGuiRender();

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

} // namespace Piece
