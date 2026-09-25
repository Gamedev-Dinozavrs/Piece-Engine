#pragma once

#include <scene/World.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Piece {

class ContentBrowserPanel {
public:
    ContentBrowserPanel();
    void OnImGuiRender();
    void SetAnimationTarget(uint32_t entityId) { m_AnimationTargetId = entityId; }
    void SetModelSpawnCallback(std::function<void(const std::filesystem::path&)> callback) {
        m_ModelSpawnCallback = std::move(callback);
    }

    const std::filesystem::path& GetAssetsDirectory() const { return m_AssetsDirectory; }

    struct UploadedObjTemplate {
        std::string name;
        std::filesystem::path sourcePath;
    };

    const std::vector<UploadedObjTemplate>& GetUploadedObjTemplates() const { return m_UploadedObjTemplates; }

private:
    void DrawAssetToolbar();
    void DrawDirectoryTree(const std::filesystem::path& directory);
    void DrawFilesystemAssets();
    void DrawUploadedTemplates();
    void DrawMaterials();
    void DrawMaterialCard(const MaterialView& material, bool isDefault);
    void LoadMaterialAssets();
    void SaveMaterialAsset(uint32_t materialId);
    void ImportModelAsset();
    void SpawnUploadedTemplate(size_t index);
    void RemoveUploadedTemplate(size_t index);
    void BeginRenameUploadedTemplate(size_t index);
    void ConfirmRenameUploadedTemplate();
    void BeginRenameMaterial(uint32_t materialId, const std::string& name);
    void CreateNewScript();
    static std::string NormalizeKey(const std::filesystem::path& path);

    std::filesystem::path m_AssetsDirectory;
    std::filesystem::path m_CurrentDirectory;
    std::unordered_map<std::string, std::vector<uint32_t>> m_LoadedAssetEntities;
    std::string m_StatusMessage;
    bool m_StatusIsError = false;
    std::vector<UploadedObjTemplate> m_UploadedObjTemplates;
    int m_SelectedUploadedTemplate = -1;
    int m_RenameTemplateIndex = -1;
    char m_RenameBuffer[128] = {};
    char m_CreateMaterialBuffer[128] = "Material";
    uint32_t m_SelectedMaterialId = 0;
    uint32_t m_RenameMaterialId = 0;
    char m_RenameMaterialBuffer[128] = {};
    bool m_OpenRenameMaterialPopup = false;
    std::filesystem::path m_SelectedAssetPath;
    bool m_MaterialAssetsLoaded = false;
    uint32_t m_AnimationTargetId = 0;
    std::function<void(const std::filesystem::path&)> m_ModelSpawnCallback;
};

} // namespace Piece
