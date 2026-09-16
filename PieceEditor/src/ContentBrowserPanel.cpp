#include "ContentBrowserPanel.h"

#include "imgui.h"

#include <assets/AssetImporter.h>
#include <core/Log.h>
#include <renderer/Renderer.h>
#include <scene/World.h>
#include <utils/platform/WindowsUtils.h>

#include <algorithm>
#include <cstdio>
#include <cctype>
#include <exception>
#include <unordered_map>
#include <vector>

namespace Piece {

namespace {

std::filesystem::path ResolveWorkspaceRoot() {
    std::filesystem::path probe = std::filesystem::current_path();
    while (!probe.empty()) {
        const bool hasCMake = std::filesystem::exists(probe / "CMakeLists.txt");
        const bool hasPieceLib = std::filesystem::exists(probe / "PieceLib");
        const bool hasPieceEditor = std::filesystem::exists(probe / "PieceEditor");
        if (hasCMake && hasPieceLib && hasPieceEditor) {
            return probe;
        }

        const std::filesystem::path parent = probe.parent_path();
        if (parent == probe) {
            break;
        }
        probe = parent;
    }

    return std::filesystem::current_path();
}

std::string GetDisplayFileName(const std::string& path) {
    if (path.empty()) {
        return "None";
    }
    return std::filesystem::path(path).filename().string();
}

void CopyDirectoryContents(const std::filesystem::path& sourceDir, const std::filesystem::path& destinationDir) {
    std::filesystem::create_directories(destinationDir);

    for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir)) {
        const auto relative = std::filesystem::relative(entry.path(), sourceDir);
        const auto target = destinationDir / relative;

        if (entry.is_directory()) {
            std::filesystem::create_directories(target);
            continue;
        }

        std::filesystem::create_directories(target.parent_path());
        std::filesystem::copy_file(entry.path(), target, std::filesystem::copy_options::overwrite_existing);
    }
}

bool FileExists(const std::string& path) {
    return !path.empty() && std::filesystem::exists(path);
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool IsSupportedModelFile(const std::filesystem::path& path) {
    const std::string ext = ToLower(path.extension().string());
    return ext == ".obj" || ext == ".gltf" || ext == ".glb";
}

} // namespace

ContentBrowserPanel::ContentBrowserPanel()
    : m_AssetsDirectory(ResolveWorkspaceRoot() / "PieceEditor" / "assets"), m_CurrentDirectory(m_AssetsDirectory) {
    if (!std::filesystem::exists(m_AssetsDirectory)) {
        std::filesystem::create_directories(m_AssetsDirectory);
    }
}

void ContentBrowserPanel::OnImGuiRender() {
    ImGui::Begin("Content Browser");

    if (!std::filesystem::exists(m_AssetsDirectory)) {
        ImGui::TextDisabled("assets directory not found: %s", m_AssetsDirectory.string().c_str());
        ImGui::End();
        return;
    }

    DrawAssetToolbar();
    DrawUploadedTemplates();

    if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F2) && m_SelectedMaterialId != 0) {
        const auto materials = World::GetMaterials();
        for (const auto& material : materials) {
            if (material.id == m_SelectedMaterialId) {
                BeginRenameMaterial(material.id, material.name);
                break;
            }
        }
    }

    bool openCreateMaterialPopup = false;
    if (ImGui::BeginPopupContextWindow("ContentBrowserEmptySpace", ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Material")) {
            std::snprintf(m_CreateMaterialBuffer, sizeof(m_CreateMaterialBuffer), "%s", "Material");
            openCreateMaterialPopup = true;
        }
        ImGui::EndPopup();
    }

    if (openCreateMaterialPopup) {
        ImGui::OpenPopup("Create Material");
    }
    if (ImGui::BeginPopupModal("Create Material", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Name", m_CreateMaterialBuffer, sizeof(m_CreateMaterialBuffer));
        if (ImGui::Button("Create")) {
            const uint32_t materialId = World::CreateMaterial(m_CreateMaterialBuffer);
            m_StatusMessage = "Created material " + std::to_string(materialId) + ".";
            m_StatusIsError = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_OpenRenameMaterialPopup) {
        ImGui::OpenPopup("Rename Material");
        m_OpenRenameMaterialPopup = false;
    }
    if (ImGui::BeginPopupModal("Rename Material", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Name", m_RenameMaterialBuffer, sizeof(m_RenameMaterialBuffer));
        if (ImGui::Button("Rename")) {
            if (World::SetMaterialName(m_RenameMaterialId, m_RenameMaterialBuffer)) {
                m_StatusMessage = "Renamed material.";
                m_StatusIsError = false;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void ContentBrowserPanel::DrawAssetToolbar() {
    ImGui::Text("Asset Root: %s", m_AssetsDirectory.string().c_str());
    ImGui::TextDisabled("Drag a material card onto an entity's Material Slot.");

    if (!m_StatusMessage.empty()) {
        if (m_StatusIsError) {
            ImGui::TextColored(ImVec4(0.95f, 0.3f, 0.3f, 1.0f), "%s", m_StatusMessage.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "%s", m_StatusMessage.c_str());
        }
    }

    if (ImGui::Button("Upload Model Placeholder")) {
        UploadModelTemplate();
    }

}

void ContentBrowserPanel::DrawMaterialCard(const MaterialView& material, bool isDefault) {
    ImGui::BeginGroup();

    const ImVec2 cardSize(96.0f, 72.0f);
    const bool selected = material.id == m_SelectedMaterialId;
    ImGui::InvisibleButton("##MaterialIcon", cardSize);
    if (ImGui::IsItemClicked()) {
        m_SelectedMaterialId = material.id;
    }
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImU32 fill = isDefault ? IM_COL32(62, 104, 146, 255) : IM_COL32(100, 78, 55, 255);
    drawList->AddRectFilled(min, max, fill, 6.0f);
    drawList->AddRect(min, max, selected ? IM_COL32(255, 220, 80, 255) : IM_COL32(180, 210, 230, 190), 6.0f, 0, selected ? 2.5f : 1.5f);
    drawList->AddCircleFilled(ImVec2((min.x + max.x) * 0.5f, min.y + 25.0f), 16.0f, IM_COL32(226, 183, 112, 255));
    drawList->AddText(ImVec2(min.x + 8.0f, min.y + 48.0f), IM_COL32(245, 245, 245, 255), "MATERIAL");

    if (ImGui::BeginDragDropSource()) {
        const uint32_t materialId = material.id;
        ImGui::SetDragDropPayload("MATERIAL_ASSET", &materialId, sizeof(materialId));
        ImGui::Text("Material: %s", material.name.c_str());
        ImGui::EndDragDropSource();
    }

    ImGui::TextWrapped("%s", material.name.c_str());
    if (isDefault) {
        ImGui::TextDisabled("Built-in");
    } else {
        ImGui::TextDisabled("Material");
    }
    ImGui::EndGroup();
}

void ContentBrowserPanel::DrawMaterials() {
    ImGui::Separator();
    ImGui::TextUnformatted("Materials");

    World::GetDefaultMaterialId();
    const auto materials = World::GetMaterials();
    if (materials.empty()) {
        ImGui::TextDisabled("No materials yet.");
        return;
    }

    const float tileWidth = 112.0f;
    const int columnCount = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / tileWidth));
    for (size_t i = 0; i < materials.size(); ++i) {
        ImGui::PushID(materials[i].id);
        DrawMaterialCard(materials[i], materials[i].id == World::GetDefaultMaterialId());
        ImGui::PopID();
        if (static_cast<int>((i + 1) % static_cast<size_t>(columnCount)) != 0) {
            ImGui::SameLine();
        }
    }
}

void ContentBrowserPanel::DrawUploadedTemplates() {
    DrawMaterials();
    ImGui::Separator();
    ImGui::TextUnformatted("Models");

    if (m_UploadedObjTemplates.empty()) {
        ImGui::TextDisabled("No uploaded models yet.");
        return;
    }

    for (size_t i = 0; i < m_UploadedObjTemplates.size(); ++i) {
        auto& uploaded = m_UploadedObjTemplates[i];
        ImGui::PushID(static_cast<int>(i));

        const bool selected = (m_SelectedUploadedTemplate == static_cast<int>(i));
        const std::string ext = ToLower(uploaded.sourcePath.extension().string());
        const std::string label = (ext.empty() ? std::string("MODEL") : ToLower(ext.substr(1))) + "  " + uploaded.name;
        if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            m_SelectedUploadedTemplate = static_cast<int>(i);
            if (ImGui::IsMouseDoubleClicked(0)) {
                SpawnUploadedTemplate(i);
            }
        }

        if (ImGui::BeginDragDropSource()) {
            const std::string pathString = uploaded.sourcePath.string();
            ImGui::SetDragDropPayload("UPLOADED_OBJ_TEMPLATE", pathString.c_str(), pathString.size() + 1);
            ImGui::TextUnformatted(uploaded.name.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("New")) {
                SpawnUploadedTemplate(i);
            }
            if (ImGui::MenuItem("Rename")) {
                BeginRenameUploadedTemplate(i);
            }
            if (ImGui::MenuItem("Remove")) {
                RemoveUploadedTemplate(i);
                ImGui::EndPopup();
                ImGui::PopID();
                continue;
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Double-click to add");
        ImGui::PopID();
    }

    if (m_RenameTemplateIndex >= 0) {
        ImGui::OpenPopup("Rename Model Placeholder");
    }

    if (ImGui::BeginPopupModal("Rename Model Placeholder", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("New Name", m_RenameBuffer, sizeof(m_RenameBuffer));
        if (ImGui::Button("OK")) {
            ConfirmRenameUploadedTemplate();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_RenameTemplateIndex = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ContentBrowserPanel::UploadModelTemplate() {
    const char* modelFilter = "Model Files\0*.obj;*.gltf;*.glb\0All Files\0*.*\0";
    Platform::OpenFileDialogAsync(modelFilter, [this](std::string selectedPath) {
        if (selectedPath.empty()) {
            return;
        }

        try {
            const std::filesystem::path source = selectedPath;
            if (!std::filesystem::exists(source)) {
                PIECE_ERROR("Selected model does not exist: {}", selectedPath);
                return;
            }

            if (!IsSupportedModelFile(source)) {
                m_StatusMessage = "Upload failed: unsupported format. Use .obj, .gltf or .glb";
                m_StatusIsError = true;
                return;
            }

            const std::filesystem::path sourceDir = source.parent_path();
            const std::filesystem::path targetDir = m_AssetsDirectory / source.stem();

            CopyDirectoryContents(sourceDir, targetDir);
            const std::filesystem::path importedModel = targetDir / source.filename();
            if (!std::filesystem::exists(importedModel)) {
                PIECE_ERROR("Imported model missing after copy: {}", importedModel.string());
                m_StatusMessage = "Upload failed: copied files but model path was not found.";
                m_StatusIsError = true;
                return;
            }

            UploadedObjTemplate uploaded{};
            uploaded.name = source.stem().string();
            uploaded.sourcePath = importedModel;
            m_UploadedObjTemplates.push_back(std::move(uploaded));
            m_SelectedUploadedTemplate = static_cast<int>(m_UploadedObjTemplates.size()) - 1;
            m_StatusMessage = "Uploaded model placeholder: " + source.filename().string();
            m_StatusIsError = false;
        } catch (const std::exception& ex) {
            PIECE_ERROR("Failed importing model into assets: {}", ex.what());
            m_StatusMessage = "Upload failed. Check filename/path and try again.";
            m_StatusIsError = true;
        }
    });
}

void ContentBrowserPanel::SpawnUploadedTemplate(size_t index) {
    if (index >= m_UploadedObjTemplates.size()) {
        m_StatusMessage = "Invalid template selection.";
        m_StatusIsError = true;
        return;
    }

    const auto& templateInfo = m_UploadedObjTemplates[index];
    ImportedModelData model;
    std::string error;
    if (!AssetImporter::ImportModel(templateInfo.sourcePath.string(), model, &error)) {
        m_StatusMessage = "Create failed: " + error;
        m_StatusIsError = true;
        return;
    }

    std::unordered_map<int, uint32_t> importerMatToWorldMat;
    importerMatToWorldMat.reserve(model.materials.size());
    for (size_t i = 0; i < model.materials.size(); ++i) {
        const ImportedMaterialData& material = model.materials[i];
        const uint32_t materialId = World::CreateMaterial(material.name.empty() ? "Imported Material" : material.name);
        World::SetMaterialSurfaceFactors(materialId, material.roughnessFactor, material.metallicFactor);
        World::SetMaterialColors(materialId, MaterialColors{material.baseColor, material.emissiveColor, material.emissiveEnabled});
        if (FileExists(material.albedoPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Albedo, material.albedoPath);
        }
        if (FileExists(material.normalPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Normal, material.normalPath);
        }
        if (FileExists(material.roughnessPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Roughness, material.roughnessPath);
        }
        if (FileExists(material.ambientOcclusionPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::AmbientOcclusion, material.ambientOcclusionPath);
        }
        if (FileExists(material.emissivePath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Emissive, material.emissivePath);
        }
        importerMatToWorldMat.emplace(static_cast<int>(i), materialId);
    }

    uint32_t spawnedCount = 0;
    float offset = 0.0f;
    for (const auto& meshData : model.meshes) {
        try {
            Ref<Mesh> mesh = CreateRef<Mesh>(Renderer::GetDevice(), meshData.vertices, meshData.indices);
            SpawnTransform spawn{};
            spawn.position = {offset, 0.0f, 0.0f};
            const uint32_t entityId = World::SpawnMesh(mesh, spawn, meshData.name.empty() ? templateInfo.name : meshData.name, meshData.hasNormals);
            if (entityId != 0) {
                auto itMat = importerMatToWorldMat.find(meshData.materialIndex);
                if (itMat != importerMatToWorldMat.end()) {
                    World::SetEntityMaterial(entityId, itMat->second);
                }
                ++spawnedCount;
            }
            offset += 1.5f;
        } catch (const std::exception& ex) {
            PIECE_ERROR("Failed to create GPU mesh for {}: {}", meshData.name, ex.what());
        }
    }

    if (spawnedCount == 0) {
        m_StatusMessage = "Create failed: no meshes were spawned.";
        m_StatusIsError = true;
        return;
    }

    m_StatusMessage = "Created " + std::to_string(spawnedCount) + " mesh(es) in scene.";
    m_StatusIsError = false;
}

void ContentBrowserPanel::RemoveUploadedTemplate(size_t index) {
    if (index >= m_UploadedObjTemplates.size()) {
        return;
    }

    m_UploadedObjTemplates.erase(m_UploadedObjTemplates.begin() + static_cast<std::ptrdiff_t>(index));
    if (m_SelectedUploadedTemplate >= static_cast<int>(m_UploadedObjTemplates.size())) {
        m_SelectedUploadedTemplate = static_cast<int>(m_UploadedObjTemplates.size()) - 1;
    }
    m_StatusMessage = "Removed uploaded model placeholder.";
    m_StatusIsError = false;
}

void ContentBrowserPanel::BeginRenameUploadedTemplate(size_t index) {
    if (index >= m_UploadedObjTemplates.size()) {
        return;
    }

    m_RenameTemplateIndex = static_cast<int>(index);
    std::snprintf(m_RenameBuffer, sizeof(m_RenameBuffer), "%s", m_UploadedObjTemplates[index].name.c_str());
}

void ContentBrowserPanel::ConfirmRenameUploadedTemplate() {
    if (m_RenameTemplateIndex < 0 || m_RenameTemplateIndex >= static_cast<int>(m_UploadedObjTemplates.size())) {
        m_RenameTemplateIndex = -1;
        return;
    }

    m_UploadedObjTemplates[static_cast<size_t>(m_RenameTemplateIndex)].name = m_RenameBuffer;
    m_StatusMessage = "Renamed uploaded model placeholder.";
    m_StatusIsError = false;
    m_RenameTemplateIndex = -1;
}

void ContentBrowserPanel::BeginRenameMaterial(uint32_t materialId, const std::string& name) {
    m_RenameMaterialId = materialId;
    std::snprintf(m_RenameMaterialBuffer, sizeof(m_RenameMaterialBuffer), "%s", name.c_str());
    m_OpenRenameMaterialPopup = true;
}

std::string ContentBrowserPanel::NormalizeKey(const std::filesystem::path& path) {
    return path.lexically_normal().string();
}

} // namespace Piece
