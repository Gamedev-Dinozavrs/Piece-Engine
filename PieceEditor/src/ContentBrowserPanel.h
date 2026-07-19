#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Piece {

class ContentBrowserPanel {
public:
    ContentBrowserPanel();
    void OnImGuiRender();

    const std::filesystem::path& GetAssetsDirectory() const { return m_AssetsDirectory; }

    struct UploadedObjTemplate {
        std::string name;
        std::filesystem::path sourcePath;
    };

    const std::vector<UploadedObjTemplate>& GetUploadedObjTemplates() const { return m_UploadedObjTemplates; }

private:
    void DrawAssetToolbar();
    void DrawMaterialTools();
    void DrawUploadedTemplates();
    void UploadModelTemplate();
    void SpawnUploadedTemplate(size_t index);
    void RemoveUploadedTemplate(size_t index);
    void BeginRenameUploadedTemplate(size_t index);
    void ConfirmRenameUploadedTemplate();
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
    char m_NewMaterialName[128] = "Material";
};

} // namespace Piece
