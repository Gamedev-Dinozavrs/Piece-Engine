#include "EditorLayer.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_vulkan.h"
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

namespace {

Entity FindEntityByUUID(const Ref<Scene>& scene, UUID uuid) {
    if (!scene || static_cast<uint64_t>(uuid) == 0) {
        return {};
    }
    auto view = scene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (view.get<TagComponent>(handle).id == uuid) {
            return Entity{handle, scene.get()};
        }
    }
    return {};
}

bool TransformChanged(const TransformComponent& left, const TransformComponent& right) {
    return left.position.x != right.position.x || left.position.y != right.position.y || left.position.z != right.position.z
        || left.rotation.x != right.rotation.x || left.rotation.y != right.rotation.y || left.rotation.z != right.rotation.z
        || left.scale.x != right.scale.x || left.scale.y != right.scale.y || left.scale.z != right.scale.z;
}

} // namespace

EditorLayer::EditorLayer()
    : Layer("EditorLayer") {
}

EditorLayer::~EditorLayer() {
}

void EditorLayer::OnAttach() {
    m_EditorScene = World::GetActiveScene();
    m_SceneHierarchyPanel.SetContext(m_EditorScene);
    m_ContentBrowserPanel.SetModelSpawnCallback([this](const std::filesystem::path& path) {
        m_SceneHierarchyPanel.SpawnModelFromPath(path);
    });

    m_PlayIcon = CreateRef<Texture>(Renderer::GetDevice(), "PieceEditor/assets/icons/play-button.png");
    m_PauseIcon = CreateRef<Texture>(Renderer::GetDevice(), "PieceEditor/assets/icons/pause-button.png");
    m_StopIcon = CreateRef<Texture>(Renderer::GetDevice(), "PieceEditor/assets/icons/stop-button.png");
    m_PlayIconDescriptor = ImGui_ImplVulkan_AddTexture(
        m_PlayIcon->getSampler(), m_PlayIcon->getImageView(), m_PlayIcon->getImageLayout());
    m_PauseIconDescriptor = ImGui_ImplVulkan_AddTexture(
        m_PauseIcon->getSampler(), m_PauseIcon->getImageView(), m_PauseIcon->getImageLayout());
    m_StopIconDescriptor = ImGui_ImplVulkan_AddTexture(
        m_StopIcon->getSampler(), m_StopIcon->getImageView(), m_StopIcon->getImageLayout());
}

void EditorLayer::OnDetach() {
    StopPlay();
    Renderer::WaitIdle();
    if (m_PlayIconDescriptor != VK_NULL_HANDLE) ImGui_ImplVulkan_RemoveTexture(m_PlayIconDescriptor);
    if (m_PauseIconDescriptor != VK_NULL_HANDLE) ImGui_ImplVulkan_RemoveTexture(m_PauseIconDescriptor);
    if (m_StopIconDescriptor != VK_NULL_HANDLE) ImGui_ImplVulkan_RemoveTexture(m_StopIconDescriptor);
    m_PlayIconDescriptor = VK_NULL_HANDLE;
    m_PauseIconDescriptor = VK_NULL_HANDLE;
    m_StopIconDescriptor = VK_NULL_HANDLE;
    m_PlayIcon.reset();
    m_PauseIcon.reset();
    m_StopIcon.reset();
}

void EditorLayer::OnUpdate(Timestep ts) {
    if (m_SceneState == SceneState::Play && m_RuntimeScene) {
        m_RuntimeScene->OnUpdateRuntime(ts);
    }
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
    const bool controlPressed = Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl);
    const bool editingText = ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantTextInput;

    if (controlPressed && !editingText && key == KeyCode::Z) {
        Undo();
        return true;
    }
    if (controlPressed && !editingText && key == KeyCode::Y) {
        Redo();
        return true;
    }

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
    const char* editorPanelNames[] = {"Hierarchy", "Properties", "Content Browser", "##SimulationToolbar"};
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

    DrawSimulationToolbar();

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
            m_SceneHierarchyPanel.SelectHierarchyRootByUUID(picked);
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
                StopPlay();
                World::ClearScene();
                m_EditorScene.reset();
                m_TransformHistory.clear();
                m_TransformHistoryCursor = 0;
                m_CurrentScenePath.clear();
                m_SceneHierarchyPanel.SetContext({});
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            const bool canUndo = m_SceneState == SceneState::Edit && m_TransformHistoryCursor > 0;
            const bool canRedo = m_SceneState == SceneState::Edit && m_TransformHistoryCursor < m_TransformHistory.size();
            if (ImGui::MenuItem("Undo Transform", "Ctrl+Z", false, canUndo)) {
                Undo();
            }
            if (ImGui::MenuItem("Redo Transform", "Ctrl+Y", false, canRedo)) {
                Redo();
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

    const TransformComponent transformBeforeManipulation = selected.GetComponent<TransformComponent>();
    glm::mat4 transform = transformBeforeManipulation.GetTransform();
    const EditorCamera& camera = Renderer::GetEditorCamera();
    glm::mat4 gizmoProjection = camera.projection();
    gizmoProjection[1][1] *= -1.0f;
    const ImGuizmo::OPERATION operations[] = {
        ImGuizmo::TRANSLATE,
        ImGuizmo::ROTATE,
        ImGuizmo::SCALE};

    const bool manipulated = ImGuizmo::Manipulate(
            &camera.view()[0][0],
            &gizmoProjection[0][0],
            operations[m_GizmoOperation],
            ImGuizmo::LOCAL,
            &transform[0][0]);
    const bool usingGizmo = ImGuizmo::IsUsing();
    if (usingGizmo && !m_GizmoWasUsing) {
        m_GizmoEntityId = selected.GetComponent<TagComponent>().id;
        m_GizmoStartTransform = transformBeforeManipulation;
    }
    if (manipulated) {
        float translation[3]{};
        float rotation[3]{};
        float scale[3]{};
        ImGuizmo::DecomposeMatrixToComponents(&transform[0][0], translation, rotation, scale);

        auto& entityTransform = selected.GetComponent<TransformComponent>();
        entityTransform.position = glm::vec3(translation[0], translation[1], translation[2]);
        entityTransform.rotation = glm::vec3(rotation[0], rotation[1], rotation[2]);
        entityTransform.scale = glm::vec3(scale[0], scale[1], scale[2]);
    }
    if (!usingGizmo && m_GizmoWasUsing && m_SceneState == SceneState::Edit) {
        Entity changedEntity = FindEntityByUUID(World::GetActiveScene(), m_GizmoEntityId);
        if (changedEntity && changedEntity.HasComponent<TransformComponent>()) {
            PushTransformCommand({m_GizmoEntityId, m_GizmoStartTransform, changedEntity.GetComponent<TransformComponent>()});
        }
    }
    m_GizmoWasUsing = usingGizmo;
}

void EditorLayer::DrawSimulationToolbar() {
    constexpr float buttonSize = 15.0f;
    constexpr float toolbarHeight = 21.0f;
    constexpr float buttonSpacing = 2.0f;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float left = viewport->WorkPos.x;
    float right = viewport->WorkPos.x + viewport->WorkSize.x;
    float top = viewport->WorkPos.y;
    if (ImGuiWindow* editorWindow = ImGui::FindWindowByName("Piece Editor")) {
        top = editorWindow->Pos.y + editorWindow->TitleBarHeight + editorWindow->MenuBarHeight;
    }
    if (ImGuiWindow* hierarchy = ImGui::FindWindowByName("Hierarchy")) {
        left = hierarchy->Pos.x + hierarchy->Size.x;
    }
    if (ImGuiWindow* properties = ImGui::FindWindowByName("Properties")) {
        right = properties->Pos.x;
    }
    if (right <= left) {
        left = viewport->WorkPos.x;
        right = viewport->WorkPos.x + viewport->WorkSize.x;
    }

    ImGui::SetNextWindowPos(ImVec2(left, top), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(right - left, toolbarHeight), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
    const ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("##SimulationToolbar", nullptr, toolbarFlags);
    const float controlsWidth = buttonSize * 3.0f + buttonSpacing * 2.0f;
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), (ImGui::GetContentRegionAvail().x - controlsWidth) * 0.5f));
    ImGui::SetCursorPosY(1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0f, 1.0f));
    const bool playHighlighted = m_SceneState == SceneState::Play;
    if (playHighlighted) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.86f, 0.36f, 0.08f, 1.0f));
    }
    ImGui::BeginDisabled(m_SceneState != SceneState::Edit);
    if (ImGui::ImageButton("##Play", ImTextureRef(reinterpret_cast<void*>(m_PlayIconDescriptor)), ImVec2(buttonSize, buttonSize))) {
        StartPlay();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Play");
    if (playHighlighted) {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();

    const bool pauseHighlighted = m_SceneState == SceneState::Pause;
    if (pauseHighlighted) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.28f, 0.07f, 1.0f));
    }
    ImGui::BeginDisabled(m_SceneState == SceneState::Edit);
    if (ImGui::ImageButton("##Pause", ImTextureRef(reinterpret_cast<void*>(m_PauseIconDescriptor)), ImVec2(buttonSize, buttonSize))) {
        TogglePause();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip(m_SceneState == SceneState::Pause ? "Resume" : "Pause");
    }
    if (pauseHighlighted) {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    if (ImGui::ImageButton("##Stop", ImTextureRef(reinterpret_cast<void*>(m_StopIconDescriptor)), ImVec2(buttonSize, buttonSize))) {
        StopPlay();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Stop");
    ImGui::PopStyleVar();
    ImGui::End();
    ImGui::PopStyleVar(4);
}

void EditorLayer::StartPlay() {
    if (m_SceneState != SceneState::Edit) {
        return;
    }
    m_EditorScene = World::GetActiveScene();
    if (!m_EditorScene) {
        return;
    }

    Entity selected = m_SceneHierarchyPanel.GetSelectedEntity();
    const UUID selectedId = selected && selected.HasComponent<TagComponent>() ? selected.GetComponent<TagComponent>().id : UUID{0};
    m_RuntimeScene = Scene::Copy(m_EditorScene);
    auto animatorView = m_RuntimeScene->GetAllEntitiesViewWith<AnimatorComponent>();
    for (auto handle : animatorView) {
        auto& animator = animatorView.get<AnimatorComponent>(handle);
        animator.time = 0.0f;
        animator.currentState = -1;
        animator.stateTime = 0.0f;
        animator.previousState = -1;
        animator.blendElapsed = 0.0f;
    }

    Renderer::WaitIdle();
    World::SetActiveScene(m_RuntimeScene);
    m_SceneHierarchyPanel.SetContext(m_RuntimeScene);
    m_SceneHierarchyPanel.SelectEntityByUUID(selectedId);
    m_SceneState = SceneState::Play;
}

void EditorLayer::TogglePause() {
    if (m_SceneState == SceneState::Play) {
        m_SceneState = SceneState::Pause;
    } else if (m_SceneState == SceneState::Pause) {
        m_SceneState = SceneState::Play;
    }
}

void EditorLayer::StopPlay() {
    if (m_SceneState == SceneState::Edit) {
        return;
    }

    Entity selected = m_SceneHierarchyPanel.GetSelectedEntity();
    const UUID selectedId = selected && selected.HasComponent<TagComponent>() ? selected.GetComponent<TagComponent>().id : UUID{0};
    Renderer::WaitIdle();
    World::SetActiveScene(m_EditorScene);
    m_RuntimeScene.reset();
    m_SceneHierarchyPanel.SetContext(m_EditorScene);
    m_SceneHierarchyPanel.SelectEntityByUUID(selectedId);
    m_SceneState = SceneState::Edit;
}

void EditorLayer::PushTransformCommand(const TransformCommand& command) {
    if (!TransformChanged(command.before, command.after)) {
        return;
    }
    m_TransformHistory.erase(m_TransformHistory.begin() + static_cast<std::ptrdiff_t>(m_TransformHistoryCursor), m_TransformHistory.end());
    m_TransformHistory.push_back(command);
    m_TransformHistoryCursor = m_TransformHistory.size();
}

bool EditorLayer::ApplyTransformCommand(const TransformCommand& command, bool useAfter) {
    if (m_SceneState != SceneState::Edit) {
        return false;
    }
    Entity entity = FindEntityByUUID(m_EditorScene, command.entityId);
    if (!entity || !entity.HasComponent<TransformComponent>()) {
        return false;
    }
    entity.GetComponent<TransformComponent>() = useAfter ? command.after : command.before;
    return true;
}

void EditorLayer::Undo() {
    if (m_SceneState != SceneState::Edit || m_TransformHistoryCursor == 0) {
        return;
    }
    --m_TransformHistoryCursor;
    ApplyTransformCommand(m_TransformHistory[m_TransformHistoryCursor], false);
}

void EditorLayer::Redo() {
    if (m_SceneState != SceneState::Edit || m_TransformHistoryCursor >= m_TransformHistory.size()) {
        return;
    }
    ApplyTransformCommand(m_TransformHistory[m_TransformHistoryCursor], true);
    ++m_TransformHistoryCursor;
}

namespace {
constexpr const char* kSceneFileFilter = "Piece Scene\0*.piecescene\0All Files\0*.*\0";
constexpr const char* kSceneFileExtension = "piecescene";
} // namespace

void EditorLayer::NewScene() {
    StopPlay();
    Renderer::WaitIdle();
    World::ClearScene();
    m_CurrentScenePath.clear();
    m_EditorScene = World::GetActiveScene();
    m_TransformHistory.clear();
    m_TransformHistoryCursor = 0;
    m_SceneHierarchyPanel.SetContext(m_EditorScene);
}

void EditorLayer::OpenScene() {
    StopPlay();
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
    m_EditorScene = World::GetActiveScene();
    m_TransformHistory.clear();
    m_TransformHistoryCursor = 0;
    m_SceneHierarchyPanel.SetContext(m_EditorScene);
}

void EditorLayer::SaveScene() {
    if (m_CurrentScenePath.empty()) {
        SaveSceneAs();
        return;
    }

    Ref<Scene> scene = m_SceneState == SceneState::Edit ? World::GetActiveScene() : m_EditorScene;
    if (!scene) {
        return;
    }

    SceneSerializer serializer(scene);
    serializer.Serialize(m_CurrentScenePath);
}

void EditorLayer::SaveSceneAs() {
    Ref<Scene> scene = m_SceneState == SceneState::Edit ? World::GetActiveScene() : m_EditorScene;
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

