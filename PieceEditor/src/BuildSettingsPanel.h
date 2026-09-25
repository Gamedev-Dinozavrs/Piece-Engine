#pragma once

#include <string>

namespace Piece {

// Editor tooling: lets the user configure and trigger a packaged build of the Sandbox game.
class BuildSettingsPanel {
public:
    void OnImGuiRender();
    void Open();

private:
    void Build();
    void CopyPackagedFiles();
    void LoadSettings();
    void SaveSettings();

    bool m_IsOpen = false;
    bool m_SettingsLoaded = false;
    bool m_IsBuilding = false;
    bool m_LastBuildSucceeded = false;
    bool m_HasBuilt = false;
    int m_Configuration = 0; // 0 = Debug, 1 = Release
    std::string m_OutputDirectory;
    std::string m_BuildLog;
};

} // namespace Piece
