#include "EditorLayer.h"
#include "EditorPlacement.h"

#include "imgui.h"
#include <scene/World.h>
#include <utils/platform/WindowsUtils.h>

#include <filesystem>
#include <string>

namespace Piece {

namespace {
    const char* PrimitiveTypeLabel(PrimitiveType primitiveType) {
        switch (primitiveType) {
        case PrimitiveType::Quad:
            return "Quad";
        case PrimitiveType::Cube:
            return "Cube";
        case PrimitiveType::Sphere:
            return "Sphere";
        default:
            return "Unknown";
        }
    }

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
    static int selectedPrimitiveIndex = 0;
    static char newMaterialName[128] = "Material";

    const char* primitiveOptions[] = {"Quad", "Cube", "Sphere"};
    ImGui::Text("Spawn Primitive");
    ImGui::SetNextItemWidth(160.0f);
    ImGui::Combo("##primitive-type", &selectedPrimitiveIndex, primitiveOptions, IM_ARRAYSIZE(primitiveOptions));
    ImGui::SameLine();
    if (ImGui::Button("Spawn")) {
        PrimitiveType primitiveType = PrimitiveType::Quad;
        if (selectedPrimitiveIndex == 1) {
            primitiveType = PrimitiveType::Cube;
        } else if (selectedPrimitiveIndex == 2) {
            primitiveType = PrimitiveType::Sphere;
        }
        EditorPlacement::SpawnPrimitiveInView(primitiveType);
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Lighting")) {
        auto lighting = World::GetLightingSettings();
        bool changed = false;

        if (ImGui::Button("Create Point Light")) {
            EditorPlacement::SpawnPointLightInView();
            lighting = World::GetLightingSettings();
        }
        ImGui::SameLine();
        if (!lighting.directionalEnabled) {
            if (ImGui::Button("Create Directional Light")) {
                lighting.directionalEnabled = true;
                lighting.directionalDirection = {-0.4f, -1.0f, -0.2f};
                lighting.directionalColor = {1.0f, 0.98f, 0.9f};
                lighting.directionalIntensity = 1.2f;
                World::SetLightingSettings(lighting);
                lighting = World::GetLightingSettings();
            }
        } else {
            if (ImGui::Button("Reset Directional")) {
                lighting.directionalEnabled = true;
                lighting.directionalDirection = {-0.4f, -1.0f, -0.2f};
                lighting.directionalColor = {1.0f, 0.98f, 0.9f};
                lighting.directionalIntensity = 1.2f;
                World::SetLightingSettings(lighting);
                lighting = World::GetLightingSettings();
            }
        }
        ImGui::SameLine();
        if (lighting.directionalEnabled) {
            if (ImGui::Button("Remove Directional")) {
                lighting.directionalEnabled = false;
                changed = true;
            }
            ImGui::SameLine();
        }
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

        if (lighting.directionalEnabled) {
            ImGui::Text("Directional Light");
            changed |= ImGui::DragFloat3("Direction", &lighting.directionalDirection.x, 0.02f, -1.0f, 1.0f, "%.2f");
            changed |= ImGui::ColorEdit3("Direction Color", &lighting.directionalColor.x);
            changed |= ImGui::DragFloat("Direction Intensity", &lighting.directionalIntensity, 0.05f, 0.0f, 50.0f, "%.2f");
        } else {
            ImGui::TextDisabled("No directional light");
        }

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
            World::SetLightingSettings(lighting);
        }

        ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::Text("Materials");
    ImGui::InputText("Material Name", newMaterialName, sizeof(newMaterialName));
    if (ImGui::Button("Create Material")) {
        World::CreateMaterial(newMaterialName);
    }

    const char* imageFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files\0*.*\0";
    auto materials = World::GetMaterials();
    if (materials.empty()) {
        ImGui::TextDisabled("No materials created");
    }

    for (auto& material : materials) {
        ImGui::PushID(static_cast<int>(material.id));
        if (ImGui::TreeNode(material.name.c_str())) {
            ImGui::Text("Albedo: %s", GetDisplayFileName(material.textures.albedoPath).c_str());
            if (ImGui::Button("Set Albedo")) {
                std::string selected = Platform::OpenFileDialog(imageFilter);
                if (!selected.empty()) {
                    World::SetMaterialTexturePath(material.id, TextureSlot::Albedo, selected);
                }
            }

            ImGui::Text("Normal: %s", GetDisplayFileName(material.textures.normalPath).c_str());
            if (ImGui::Button("Set Normal")) {
                std::string selected = Platform::OpenFileDialog(imageFilter);
                if (!selected.empty()) {
                    World::SetMaterialTexturePath(material.id, TextureSlot::Normal, selected);
                }
            }

            ImGui::Text("Height: %s", GetDisplayFileName(material.textures.heightPath).c_str());
            if (ImGui::Button("Set Height")) {
                std::string selected = Platform::OpenFileDialog(imageFilter);
                if (!selected.empty()) {
                    World::SetMaterialTexturePath(material.id, TextureSlot::Height, selected);
                }
            }

            ImGui::Text("Roughness: %s", GetDisplayFileName(material.textures.roughnessPath).c_str());
            if (ImGui::Button("Set Roughness")) {
                std::string selected = Platform::OpenFileDialog(imageFilter);
                if (!selected.empty()) {
                    World::SetMaterialTexturePath(material.id, TextureSlot::Roughness, selected);
                }
            }

            ImGui::Text("Ambient Occlusion: %s", GetDisplayFileName(material.textures.ambientOcclusionPath).c_str());
            if (ImGui::Button("Set AO")) {
                std::string selected = Platform::OpenFileDialog(imageFilter);
                if (!selected.empty()) {
                    World::SetMaterialTexturePath(material.id, TextureSlot::AmbientOcclusion, selected);
                }
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::Text("Objects");
    auto entities = World::GetRenderEntities();
    if (entities.empty()) {
        ImGui::TextDisabled("No renderable objects in scene");
    }

    for (auto& entity : entities) {
        ImGui::PushID(static_cast<int>(entity.id));
        std::string label = entity.name + " (" + PrimitiveTypeLabel(entity.primitiveType) + ")";
        if (ImGui::TreeNode(label.c_str())) {
            bool transformChanged = false;
            transformChanged |= ImGui::DragFloat3("Position", &entity.transform.position.x, 0.05f, -50.0f, 50.0f, "%.2f");
            transformChanged |= ImGui::DragFloat3("Rotation", &entity.transform.rotation.x, 0.5f, -360.0f, 360.0f, "%.1f");
            transformChanged |= ImGui::DragFloat3("Scale", &entity.transform.scale.x, 0.05f, 0.01f, 100.0f, "%.2f");
            if (transformChanged) {
                World::SetEntityTransform(entity.id, entity.transform);
            }

            if (materials.empty()) {
                ImGui::TextDisabled("No materials available");
            } else {
                int currentMaterialIndex = -1;
                for (size_t i = 0; i < materials.size(); ++i) {
                    if (materials[i].id == entity.materialId) {
                        currentMaterialIndex = static_cast<int>(i);
                        break;
                    }
                }

                const char* preview = "None";
                if (currentMaterialIndex >= 0) {
                    preview = materials[static_cast<size_t>(currentMaterialIndex)].name.c_str();
                }

                if (ImGui::BeginCombo("Material", preview)) {
                    if (ImGui::Selectable("None", currentMaterialIndex == -1)) {
                        World::SetEntityMaterial(entity.id, 0);
                        currentMaterialIndex = -1;
                    }

                    for (size_t i = 0; i < materials.size(); ++i) {
                        const bool selected = currentMaterialIndex == static_cast<int>(i);
                        if (ImGui::Selectable(materials[i].name.c_str(), selected)) {
                            World::SetEntityMaterial(entity.id, materials[i].id);
                            currentMaterialIndex = static_cast<int>(i);
                        }
                    }

                    ImGui::EndCombo();
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
