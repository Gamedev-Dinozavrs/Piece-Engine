#include "EditorLayer.h"

#include "imgui.h"
#include <renderer/Renderer.h>
#include <utils/platform/WindowsUtils.h>

#include <filesystem>
#include <string>

namespace Piece {

namespace {
    std::string GetDisplayFileName(const std::string& path) {
        if (path.empty()) {
            return "None";
        }

        return std::filesystem::path(path).filename().string();
    }
}

EditorLayer::EditorLayer()
    : Layer("EditorLayer") {
}

EditorLayer::~EditorLayer() {
}

void EditorLayer::OnAttach() {
}

void EditorLayer::OnDetach() {
}

void EditorLayer::OnUpdate(Timestep ts) {
    (void)ts;
}

void EditorLayer::OnEvent(Event& event) {
    (void)event;
}

void EditorLayer::OnImGuiRender() {
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
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();

    ImGui::Begin("Scene");
    ImGui::Text("Editor skeleton");
    if (ImGui::Button("Create Quad")) {
        Renderer::CreateQuadInView();
    }
    ImGui::SameLine();
    if (ImGui::Button("Create Cube")) {
        Renderer::CreateCubeInView();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Lighting")) {
        auto lighting = Renderer::GetLightingSettings();
        bool changed = false;

        if (ImGui::Button("Create Point Light")) {
            Renderer::CreatePointLightInView();
            lighting = Renderer::GetLightingSettings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Directional")) {
            Renderer::ResetDirectionalLight();
            lighting = Renderer::GetLightingSettings();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(true);
        ImGui::Button("Create Spot Light");
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Spot lights are not supported yet.");
        }

        ImGui::Text("Point Lights: %u/%u", lighting.pointLightCount, static_cast<uint32_t>(lighting.pointLights.size()));
        ImGui::SameLine();
        if (ImGui::Button("Remove Last Point")) {
            if (lighting.pointLightCount > 0) {
                lighting.pointLightCount -= 1;
                lighting.pointLights[lighting.pointLightCount] = PointLightSettings{};
                changed = true;
            }
        }

        ImGui::Text("Directional Light");
        changed |= ImGui::DragFloat3("Direction", &lighting.directionalDirection.x, 0.02f, -1.0f, 1.0f, "%.2f");
        changed |= ImGui::ColorEdit3("Direction Color", &lighting.directionalColor.x);
        changed |= ImGui::DragFloat("Direction Intensity", &lighting.directionalIntensity, 0.05f, 0.0f, 50.0f, "%.2f");

        ImGui::Separator();
        ImGui::Text("Specular");
        changed |= ImGui::DragFloat("Specular Strength", &lighting.specularStrength, 0.02f, 0.0f, 8.0f, "%.2f");
        changed |= ImGui::DragFloat("Shininess Min", &lighting.specularShininessMin, 0.2f, 1.0f, 256.0f, "%.1f");
        changed |= ImGui::DragFloat("Shininess Max", &lighting.specularShininessMax, 0.2f, 1.0f, 512.0f, "%.1f");

        for (uint32_t i = 0; i < lighting.pointLightCount && i < lighting.pointLights.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            std::string nodeLabel = "Point Light #" + std::to_string(i);
            if (ImGui::TreeNode(nodeLabel.c_str())) {
                changed |= ImGui::DragFloat3("Position", &lighting.pointLights[i].position.x, 0.05f, -50.0f, 50.0f, "%.2f");
                changed |= ImGui::DragFloat("Radius", &lighting.pointLights[i].radius, 0.05f, 0.01f, 100.0f, "%.2f");
                changed |= ImGui::ColorEdit3("Color", &lighting.pointLights[i].color.x);
                changed |= ImGui::DragFloat("Intensity", &lighting.pointLights[i].intensity, 0.05f, 0.0f, 100.0f, "%.2f");
                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        if (changed) {
            Renderer::SetLightingSettings(lighting);
        }

        ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::Text("Quads");

    auto quadMaterials = Renderer::GetQuadMaterials();
    if (quadMaterials.empty()) {
        ImGui::TextDisabled("No quads in scene");
    }

    const char* imageFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files\0*.*\0";

    for (const auto& quad : quadMaterials) {
        ImGui::PushID(static_cast<int>(quad.id));

        std::string label = "Quad #" + std::to_string(quad.id);
        if (ImGui::TreeNode(label.c_str())) {
            ImGui::Text("Albedo: %s", GetDisplayFileName(quad.albedoPath).c_str());
            if (ImGui::Button("Upload Albedo")) {
                std::string selected = Platform::OpenFileDialog(imageFilter);
                if (!selected.empty()) {
                    Renderer::SetQuadTexturePath(quad.id, TextureSlot::Albedo, selected);
                }
            }

            ImGui::Text("Normal (Blue): %s", GetDisplayFileName(quad.normalPath).c_str());
            if (ImGui::Button("Upload Normal")) {
                std::string selected = Platform::OpenFileDialog(imageFilter);
                if (!selected.empty()) {
                    Renderer::SetQuadTexturePath(quad.id, TextureSlot::Normal, selected);
                }
            }

            ImGui::Text("Height: %s", GetDisplayFileName(quad.heightPath).c_str());
            if (ImGui::Button("Upload Height")) {
                std::string selected = Platform::OpenFileDialog(imageFilter);
                if (!selected.empty()) {
                    Renderer::SetQuadTexturePath(quad.id, TextureSlot::Height, selected);
                }
            }

            ImGui::Text("Roughness: %s", GetDisplayFileName(quad.roughnessPath).c_str());
            if (ImGui::Button("Upload Roughness")) {
                std::string selected = Platform::OpenFileDialog(imageFilter);
                if (!selected.empty()) {
                    Renderer::SetQuadTexturePath(quad.id, TextureSlot::Roughness, selected);
                }
            }

            ImGui::Text("Ambient Occlusion: %s", GetDisplayFileName(quad.ambientOcclusionPath).c_str());
            if (ImGui::Button("Upload AO")) {
                std::string selected = Platform::OpenFileDialog(imageFilter);
                if (!selected.empty()) {
                    Renderer::SetQuadTexturePath(quad.id, TextureSlot::AmbientOcclusion, selected);
                }
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }
    ImGui::End();

    if (m_ShowMetrics) {
        ImGui::ShowMetricsWindow(&m_ShowMetrics);
    }
}

} // namespace Piece
