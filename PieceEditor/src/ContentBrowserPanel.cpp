#include "ContentBrowserPanel.h"

#include "EditorPlacement.h"

#include "imgui.h"

#include <assets/AssetImporter.h>
#include <assets/MaterialAssetSerializer.h>
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

std::string MakeSafeFileName(std::string value) {
    for (char& character : value) {
        if (character == '<' || character == '>' || character == ':' || character == '"'
            || character == '/' || character == '\\' || character == '|' || character == '?'
            || character == '*') {
            character = '_';
        }
    }
    return value.empty() ? "Material" : value;
}

enum class AssetTileKind {
    Material,
    Model,
    Scene,
    Texture,
    Animation,
    Folder
};

bool IsSupportedModelFile(const std::filesystem::path& path) {
    const std::string ext = ToLower(path.extension().string());
    return ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx";
}

AssetTileKind GetAssetTileKind(const std::filesystem::path& path) {
    if (std::filesystem::is_directory(path)) {
        return AssetTileKind::Folder;
    }
    const std::string ext = ToLower(path.extension().string());
    if (ext == ".piecescene") {
        return AssetTileKind::Scene;
    }
    if (ext == ".piece-material") {
        return AssetTileKind::Material;
    }
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".hdr") {
        return AssetTileKind::Texture;
    }
    if (ext == ".anim" || ext == ".animation") {
        return AssetTileKind::Animation;
    }
    return AssetTileKind::Model;
}

void DrawAssetTileIcon(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, AssetTileKind kind, bool selected) {
    const ImU32 background = selected ? IM_COL32(44, 76, 104, 255) : IM_COL32(31, 39, 48, 255);
    const ImU32 outline = selected ? IM_COL32(255, 214, 96, 255) : IM_COL32(94, 113, 132, 255);
    const ImU32 accent = kind == AssetTileKind::Material ? IM_COL32(222, 174, 91, 255)
        : kind == AssetTileKind::Model ? IM_COL32(102, 180, 219, 255)
        : kind == AssetTileKind::Scene ? IM_COL32(154, 188, 220, 255)
        : kind == AssetTileKind::Texture ? IM_COL32(104, 188, 132, 255)
        : kind == AssetTileKind::Animation ? IM_COL32(211, 126, 188, 255)
        : IM_COL32(191, 157, 85, 255);
    drawList->AddRectFilled(min, max, background, 6.0f);
    drawList->AddRect(min, max, outline, 6.0f, 0, selected ? 2.5f : 1.0f);

    const ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
    if (kind == AssetTileKind::Material) {
        drawList->AddCircleFilled(center, 15.0f, accent);
        drawList->AddCircle(ImVec2(center.x - 4.0f, center.y - 5.0f), 5.0f, IM_COL32(255, 235, 188, 220), 16, 2.0f);
    } else if (kind == AssetTileKind::Model) {
        const ImVec2 top(center.x, center.y - 17.0f);
        const ImVec2 left(center.x - 17.0f, center.y - 7.0f);
        const ImVec2 right(center.x + 17.0f, center.y - 7.0f);
        const ImVec2 bottom(center.x, center.y + 12.0f);
        drawList->AddQuadFilled(top, right, bottom, left, accent);
        drawList->AddLine(top, right, IM_COL32(231, 246, 255, 220), 1.5f);
        drawList->AddLine(right, bottom, IM_COL32(231, 246, 255, 220), 1.5f);
        drawList->AddLine(bottom, left, IM_COL32(231, 246, 255, 220), 1.5f);
        drawList->AddLine(left, top, IM_COL32(231, 246, 255, 220), 1.5f);
    } else if (kind == AssetTileKind::Scene) {
        const ImVec2 pageMin(center.x - 15.0f, center.y - 18.0f);
        const ImVec2 pageMax(center.x + 15.0f, center.y + 18.0f);
        drawList->AddRectFilled(pageMin, pageMax, accent, 3.0f);
        drawList->AddLine(ImVec2(center.x - 8.0f, center.y - 5.0f), ImVec2(center.x + 8.0f, center.y - 5.0f), background, 2.0f);
        drawList->AddLine(ImVec2(center.x - 8.0f, center.y + 3.0f), ImVec2(center.x + 8.0f, center.y + 3.0f), background, 2.0f);
    } else if (kind == AssetTileKind::Texture) {
        drawList->AddRectFilled(ImVec2(center.x - 18.0f, center.y - 15.0f), ImVec2(center.x + 18.0f, center.y + 15.0f), accent, 3.0f);
        drawList->AddTriangleFilled(ImVec2(center.x - 14.0f, center.y + 10.0f), ImVec2(center.x - 2.0f, center.y - 5.0f), ImVec2(center.x + 7.0f, center.y + 10.0f), background);
        drawList->AddCircleFilled(ImVec2(center.x + 9.0f, center.y - 7.0f), 4.0f, IM_COL32(255, 239, 169, 255));
    } else if (kind == AssetTileKind::Animation) {
        drawList->AddCircleFilled(center, 15.0f, accent);
        drawList->AddTriangleFilled(ImVec2(center.x - 4.0f, center.y - 8.0f), ImVec2(center.x - 4.0f, center.y + 8.0f), ImVec2(center.x + 9.0f, center.y), IM_COL32(255, 240, 255, 230));
    } else {
        drawList->AddRectFilled(ImVec2(center.x - 19.0f, center.y - 12.0f), ImVec2(center.x + 19.0f, center.y + 14.0f), accent, 3.0f);
        drawList->AddRectFilled(ImVec2(center.x - 15.0f, center.y - 17.0f), ImVec2(center.x - 1.0f, center.y - 10.0f), accent, 2.0f);
    }
}

void DrawAssetTile(const char* id, const std::string& label, AssetTileKind kind, bool selected) {
    const ImVec2 tileSize(112.0f, 104.0f);
    ImGui::InvisibleButton(id, tileSize);
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const ImVec2 iconMin(min.x + 8.0f, min.y + 8.0f);
    const ImVec2 iconMax(max.x - 8.0f, min.y + 64.0f);
    DrawAssetTileIcon(ImGui::GetWindowDrawList(), iconMin, iconMax, kind, selected);
    ImGui::GetWindowDrawList()->AddText(ImVec2(min.x + 8.0f, min.y + 72.0f), IM_COL32(232, 238, 244, 255), label.c_str());
}

} // namespace

ContentBrowserPanel::ContentBrowserPanel()
    : m_AssetsDirectory(ResolveWorkspaceRoot() / "PieceEditor" / "assets"), m_CurrentDirectory(m_AssetsDirectory) {
    if (!std::filesystem::exists(m_AssetsDirectory)) {
        std::filesystem::create_directories(m_AssetsDirectory);
    }
    LoadMaterialAssets();
}

void ContentBrowserPanel::OnImGuiRender() {
    ImGui::Begin("Content Browser");

    if (!std::filesystem::exists(m_AssetsDirectory)) {
        ImGui::TextDisabled("assets directory not found: %s", m_AssetsDirectory.string().c_str());
        ImGui::End();
        return;
    }

    DrawAssetToolbar();
    DrawFilesystemAssets();
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
        EditorPlacement::DrawCreateMenu();
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
            SaveMaterialAsset(materialId);
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
                SaveMaterialAsset(m_RenameMaterialId);
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
    ImGui::TextDisabled("Current: %s", m_CurrentDirectory.string().c_str());
    if (m_CurrentDirectory != m_AssetsDirectory && ImGui::SmallButton("Up##AssetDirectory")) {
        m_CurrentDirectory = m_CurrentDirectory.parent_path();
    }
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

void ContentBrowserPanel::DrawFilesystemAssets() {
    std::vector<std::filesystem::path> entries;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory, error)) {
        if (!error) {
            entries.push_back(entry.path());
        }
    }
    std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
        const bool leftDirectory = std::filesystem::is_directory(left);
        const bool rightDirectory = std::filesystem::is_directory(right);
        if (leftDirectory != rightDirectory) {
            return leftDirectory > rightDirectory;
        }
        return ToLower(left.filename().string()) < ToLower(right.filename().string());
    });

    ImGui::Separator();
    ImGui::TextUnformatted("Assets");
    if (entries.empty()) {
        ImGui::TextDisabled("This folder is empty.");
        return;
    }

    const float tileWidth = 112.0f;
    const int columnCount = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / tileWidth));
    for (size_t index = 0; index < entries.size(); ++index) {
        const auto& path = entries[index];
        ImGui::PushID(path.string().c_str());
        const bool selected = path == m_SelectedAssetPath;
        DrawAssetTile("##FilesystemTile", path.stem().string(), GetAssetTileKind(path), selected);
        if (ImGui::IsItemClicked()) {
            m_SelectedAssetPath = path;
            if (ImGui::IsMouseDoubleClicked(0)) {
                if (std::filesystem::is_directory(path)) {
                    m_CurrentDirectory = path;
                } else if (IsSupportedModelFile(path)) {
                    UploadedObjTemplate uploaded{};
                    uploaded.name = path.stem().string();
                    uploaded.sourcePath = path;
                    m_UploadedObjTemplates.push_back(std::move(uploaded));
                    SpawnUploadedTemplate(m_UploadedObjTemplates.size() - 1);
                } else if (ToLower(path.extension().string()) == ".piecescene") {
                    m_StatusMessage = "Scene selected: " + path.filename().string();
                    m_StatusIsError = false;
                }
            }
        }
        ImGui::PopID();
        if (static_cast<int>((index + 1) % static_cast<size_t>(columnCount)) != 0) {
            ImGui::SameLine();
        }
    }
}

void ContentBrowserPanel::DrawMaterialCard(const MaterialView& material, bool isDefault) {
    ImGui::BeginGroup();

    const bool selected = material.id == m_SelectedMaterialId;
    DrawAssetTile("##MaterialTile", material.name, AssetTileKind::Material, selected);
    if (ImGui::IsItemClicked()) {
        m_SelectedMaterialId = material.id;
    }

    if (ImGui::BeginDragDropSource()) {
        const uint32_t materialId = material.id;
        ImGui::SetDragDropPayload("MATERIAL_ASSET", &materialId, sizeof(materialId));
        ImGui::Text("Material: %s", material.name.c_str());
        ImGui::EndDragDropSource();
    }

    if (isDefault) {
        ImGui::TextDisabled("Built-in");
    } else {
        ImGui::TextDisabled("Material");
    }
    ImGui::EndGroup();
}

void ContentBrowserPanel::LoadMaterialAssets() {
    if (m_MaterialAssetsLoaded) {
        return;
    }

    std::error_code error;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(m_AssetsDirectory, error)) {
        if (error || !entry.is_regular_file() || ToLower(entry.path().extension().string()) != ".piece-material") {
            continue;
        }

        MaterialView material{};
        if (!MaterialAssetSerializer::Deserialize(entry.path().string(), material)) {
            continue;
        }

        const uint32_t materialId = World::CreateMaterial(material.name);
        World::SetMaterialAssetPath(materialId, entry.path().string());
        World::SetMaterialColors(materialId, material.colors);
        World::SetMaterialSurfaceFactors(
            materialId,
            material.surfaceFactors.roughnessFactor,
            material.surfaceFactors.metallicFactor,
            material.surfaceFactors.normalScale,
            material.surfaceFactors.occlusionStrength);
        World::SetMaterialRenderSettings(materialId, material.renderSettings);
        World::SetMaterialTexturePath(materialId, TextureSlot::Albedo, material.textures.albedoPath);
        World::SetMaterialTexturePath(materialId, TextureSlot::Normal, material.textures.normalPath);
        World::SetMaterialTexturePath(materialId, TextureSlot::Height, material.textures.heightPath);
        World::SetMaterialTexturePath(materialId, TextureSlot::Roughness, material.textures.roughnessPath);
        World::SetMaterialTexturePath(materialId, TextureSlot::Metallic, material.textures.metallicPath);
        World::SetMaterialTexturePath(materialId, TextureSlot::AmbientOcclusion, material.textures.ambientOcclusionPath);
        World::SetMaterialTexturePath(materialId, TextureSlot::Emissive, material.textures.emissivePath);
    }

    m_MaterialAssetsLoaded = true;
}

void ContentBrowserPanel::SaveMaterialAsset(uint32_t materialId) {
    const auto materials = World::GetMaterials();
    auto materialIt = std::find_if(materials.begin(), materials.end(), [materialId](const MaterialView& material) {
        return material.id == materialId;
    });
    if (materialIt == materials.end()) {
        return;
    }

    MaterialView material = *materialIt;
    std::filesystem::path assetPath = material.assetPath;
    if (assetPath.empty()) {
        const std::filesystem::path directory = m_AssetsDirectory / "Materials";
        std::filesystem::create_directories(directory);
        assetPath = directory / (MakeSafeFileName(material.name) + ".piece-material");
        World::SetMaterialAssetPath(materialId, assetPath.string());
        material.assetPath = assetPath.string();
    }

    MaterialAssetSerializer::Serialize(material, assetPath.string());
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

    const float tileWidth = 112.0f;
    const int columnCount = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / tileWidth));
    for (size_t i = 0; i < m_UploadedObjTemplates.size(); ++i) {
        auto& uploaded = m_UploadedObjTemplates[i];
        ImGui::PushID(static_cast<int>(i));

        const bool selected = (m_SelectedUploadedTemplate == static_cast<int>(i));
        const std::string ext = ToLower(uploaded.sourcePath.extension().string());
        const std::string label = uploaded.name.empty() ? (ext.empty() ? "Model" : ext.substr(1)) : uploaded.name;
        DrawAssetTile("##ModelTile", label, AssetTileKind::Model, selected);
        if (ImGui::IsItemClicked()) {
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

        ImGui::PopID();
        if (static_cast<int>((i + 1) % static_cast<size_t>(columnCount)) != 0) {
            ImGui::SameLine();
        }
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
    const char* modelFilter = "Model Files\0*.obj;*.gltf;*.glb;*.fbx\0All Files\0*.*\0";
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

            std::error_code pathError;
            const std::filesystem::path normalizedAssets = std::filesystem::weakly_canonical(m_AssetsDirectory, pathError);
            const std::filesystem::path normalizedSource = std::filesystem::weakly_canonical(source, pathError);
            const std::filesystem::path relativeSource = std::filesystem::relative(normalizedSource, normalizedAssets, pathError);
            const std::string relativeSourceString = relativeSource.string();
            const bool alreadyInAssets = !pathError
                && relativeSource != "."
                && relativeSourceString.rfind("..", 0) != 0;

            std::filesystem::path importedModel = source;
            if (!alreadyInAssets) {
                const std::filesystem::path sourceDir = source.parent_path();
                const std::filesystem::path targetDir = m_AssetsDirectory / source.stem();
                CopyDirectoryContents(sourceDir, targetDir);
                importedModel = targetDir / source.filename();
            }

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

    if (!model.animations.empty()) {
        if (m_AnimationTargetId == 0 || !World::SetEntityAnimationClips(m_AnimationTargetId, model.animations)) {
            m_StatusMessage = "Animation loaded, but select the X Bot root first.";
            m_StatusIsError = true;
            return;
        }

        m_StatusMessage = "Applied animation: " + templateInfo.name + ".";
        m_StatusIsError = false;
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
                World::SetEntityImportedModelInfo(entityId, templateInfo.sourcePath.string(), meshData.name);
                if (!model.joints.empty()) {
                    World::SetEntityAnimationData(entityId, model.joints, model.animations);
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
