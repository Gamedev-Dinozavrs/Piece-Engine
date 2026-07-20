#include "EditorLayer.h"

#include "imgui.h"
#include <core/Input.h>
#include <core/KeyCodes.h>
#include <event/Event.h>
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

    ImGui::End();

    m_SceneHierarchyPanel.OnImGuiRender();
    m_ContentBrowserPanel.OnImGuiRender();

    if (m_ShowMetrics) {
        ImGui::ShowMetricsWindow(&m_ShowMetrics);
    }
}

} // namespace Piece
