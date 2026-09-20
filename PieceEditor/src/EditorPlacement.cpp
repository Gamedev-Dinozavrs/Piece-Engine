#include "EditorPlacement.h"

#include <imgui.h>
#include <renderer/Renderer.h>
#include <scene/World.h>

namespace Piece {

namespace EditorPlacement {

bool DrawCreateMenu() {
    bool created = false;
    if (ImGui::BeginMenu("Create")) {
        if (ImGui::MenuItem("Quad")) {
            SpawnQuadInView();
            created = true;
        }
        if (ImGui::MenuItem("Cube")) {
            SpawnCubeInView();
            created = true;
        }
        if (ImGui::MenuItem("Sphere")) {
            SpawnSphereInView();
            created = true;
        }
        if (ImGui::MenuItem("Empty Object")) {
            World::CreateEmptyObject("Entity");
            created = true;
        }
        if (ImGui::MenuItem("Point Light")) {
            created = SpawnPointLightInView();
        }
        if (ImGui::MenuItem("Directional Light")) {
            auto lighting = World::GetLightingSettings();
            lighting.directionalEnabled = true;
            World::SetLightingSettings(lighting);
            created = true;
        }
        if (ImGui::MenuItem("Environment")) {
            World::CreateEnvironmentObject();
            created = true;
        }
        ImGui::EndMenu();
    }
    return created;
}

void SpawnPrimitiveInView(PrimitiveType primitiveType) {
    switch (primitiveType) {
    case PrimitiveType::Quad:
        Renderer::CreateQuadInView();
        break;
    case PrimitiveType::Cube:
        Renderer::CreateCubeInView();
        break;
    case PrimitiveType::Sphere:
        Renderer::CreateSphereInView();
        break;
    default:
        break;
    }
}

void SpawnQuadInView() {
    SpawnPrimitiveInView(PrimitiveType::Quad);
}

void SpawnCubeInView() {
    SpawnPrimitiveInView(PrimitiveType::Cube);
}

void SpawnSphereInView() {
    SpawnPrimitiveInView(PrimitiveType::Sphere);
}

bool SpawnPointLightInView() {
    return Renderer::CreatePointLightInView();
}

} // namespace EditorPlacement

} // namespace Piece
