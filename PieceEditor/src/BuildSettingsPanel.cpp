#include "BuildSettingsPanel.h"

#include "imgui.h"

#include <utils/platform/WindowsUtils.h>

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace Piece {

namespace {
constexpr const char* kSettingsPath = ".piece_build_settings";
} // namespace

void BuildSettingsPanel::Open() {
    m_IsOpen = true;
    if (!m_SettingsLoaded) {
        LoadSettings();
        m_SettingsLoaded = true;
    }
}

void BuildSettingsPanel::LoadSettings() {
    std::ifstream file(kSettingsPath);
    if (!file.is_open()) {
        return;
    }

    std::getline(file, m_OutputDirectory);
    std::string configLine;
    if (std::getline(file, configLine) && !configLine.empty()) {
        m_Configuration = std::atoi(configLine.c_str());
    }
}

void BuildSettingsPanel::SaveSettings() {
    std::ofstream file(kSettingsPath);
    if (!file.is_open()) {
        return;
    }
    file << m_OutputDirectory << "\n" << m_Configuration << "\n";
}

void BuildSettingsPanel::OnImGuiRender() {
    if (!m_IsOpen) {
        return;
    }

    ImGui::Begin("Build Settings", &m_IsOpen);

    char buffer[512] = {};
    std::snprintf(buffer, sizeof(buffer), "%s", m_OutputDirectory.c_str());
    if (ImGui::InputText("Output Directory", buffer, sizeof(buffer))) {
        m_OutputDirectory = buffer;
        SaveSettings();
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        Platform::BrowseFolderDialogAsync("Select Build Output Folder", [this](std::string path) {
            m_OutputDirectory = std::move(path);
            SaveSettings();
        });
    }

    const char* configurations[] = { "Debug", "Release" };
    if (ImGui::Combo("Configuration", &m_Configuration, configurations, 2)) {
        SaveSettings();
    }

    ImGui::BeginDisabled(m_IsBuilding || m_OutputDirectory.empty());
    if (ImGui::Button("Build")) {
        Build();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(m_OutputDirectory.empty() || !std::filesystem::exists(m_OutputDirectory));
    if (ImGui::Button("Open Output Folder")) {
        Platform::OpenInFileExplorer(m_OutputDirectory);
    }
    ImGui::EndDisabled();

    if (m_IsBuilding) {
        ImGui::SameLine();
        ImGui::TextUnformatted("Building...");
    } else if (m_HasBuilt) {
        ImGui::SameLine();
        ImGui::TextColored(
            m_LastBuildSucceeded ? ImVec4(0.3f, 0.9f, 0.3f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f),
            "%s", m_LastBuildSucceeded ? "Success" : "Failed");
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Build Log");
    ImGui::InputTextMultiline("##BuildLog", m_BuildLog.data(), m_BuildLog.size() + 1,
        ImVec2(-1.0f, 300.0f), ImGuiInputTextFlags_ReadOnly);

    ImGui::End();
}

void BuildSettingsPanel::Build() {
    if (m_OutputDirectory.empty() || m_IsBuilding) {
        return;
    }

    m_IsBuilding = true;
    m_HasBuilt = false;
    const std::string config = m_Configuration == 0 ? "Debug" : "Release";
    m_BuildLog = "Building Sandbox (" + config + ")...\n";

    const std::string command = "cmake --build build --config " + config + " --target Sandbox";
    Platform::RunProcessAsync(command, "", [this](int exitCode, std::string output) {
        m_BuildLog += output;
        m_LastBuildSucceeded = (exitCode == 0);
        m_HasBuilt = true;
        m_IsBuilding = false;

        if (m_LastBuildSucceeded) {
            CopyPackagedFiles();
        } else {
            m_BuildLog += "\nBuild failed with exit code " + std::to_string(exitCode);
        }
    });
}

void BuildSettingsPanel::CopyPackagedFiles() {
    namespace fs = std::filesystem;
    const std::string config = m_Configuration == 0 ? "Debug" : "Release";

    try {
        const fs::path outputDir(m_OutputDirectory);
        fs::create_directories(outputDir);

        const fs::path exeSource = fs::path("build") / "Sandbox" / config / "Sandbox.exe";
        if (fs::exists(exeSource)) {
            fs::copy_file(exeSource, outputDir / "Sandbox.exe", fs::copy_options::overwrite_existing);
            m_BuildLog += "\nCopied Sandbox.exe";
        } else {
            m_BuildLog += "\nWARNING: " + exeSource.string() + " not found";
        }

        const fs::path scriptsSource = fs::path("Scripts") / "Piece.ScriptCore" / "bin";
        if (fs::exists(scriptsSource)) {
            const fs::path scriptsDest = outputDir / "Scripts" / "Piece.ScriptCore" / "bin";
            fs::create_directories(scriptsDest);
            fs::copy(scriptsSource, scriptsDest,
                fs::copy_options::overwrite_existing | fs::copy_options::recursive);
            m_BuildLog += "\nCopied managed scripts";
        }

        const fs::path shaderSource = fs::path("PieceLib") / "src" / "shaders";
        if (fs::exists(shaderSource)) {
            const fs::path shaderDest = outputDir / "shaders";
            fs::create_directories(shaderDest);
            for (const auto& entry : fs::directory_iterator(shaderSource)) {
                if (entry.path().extension() == ".spv") {
                    fs::copy_file(entry.path(), shaderDest / entry.path().filename(), fs::copy_options::overwrite_existing);
                }
            }
            m_BuildLog += "\nCopied compiled shaders";
        }

        m_BuildLog += "\nPackaged build complete: " + outputDir.string();
    } catch (const std::exception& exception) {
        m_LastBuildSucceeded = false;
        m_BuildLog += std::string("\nPackaging failed: ") + exception.what();
    }
}

} // namespace Piece
