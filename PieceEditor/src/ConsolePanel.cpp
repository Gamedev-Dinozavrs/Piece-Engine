#include "ConsolePanel.h"

#include "imgui.h"

#include <core/Application.h>
#include <core/EditorConsoleSink.h>
#include <core/Log.h>
#include <scene/Scene.h>
#include <scene/World.h>

#include <cstdio>
#include <cstring>

namespace Piece {

namespace {

ImVec4 ColorForLevel(int level) {
    switch (level) {
    case 3: return ImVec4(0.95f, 0.82f, 0.35f, 1.0f); // warn
    case 4: return ImVec4(0.95f, 0.35f, 0.35f, 1.0f); // err
    case 5: return ImVec4(1.0f, 0.2f, 0.2f, 1.0f);    // critical
    case 0: case 1: return ImVec4(0.6f, 0.6f, 0.6f, 1.0f); // trace/debug
    default: return ImVec4(0.85f, 0.85f, 0.85f, 1.0f); // info
    }
}

int TextEditCallbackStub(ImGuiInputTextCallbackData* data) {
    auto* panel = static_cast<ConsolePanel*>(data->UserData);
    return panel->TextEditCallback(data);
}

} // namespace

void ConsolePanel::OnImGuiRender() {
    ImGui::Begin("Console");

    if (ImGui::Button("Clear")) {
        EditorConsoleSink::Clear();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Type 'help' for a list of commands");

    ImGui::Separator();

    const float inputHeight = ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0.0f, -inputHeight), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const ConsoleLogEntry& entry : EditorConsoleSink::GetEntries()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ColorForLevel(entry.level));
        ImGui::TextUnformatted(entry.message.c_str());
        ImGui::PopStyleColor();
    }
    if (m_ScrollToBottom || ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    m_ScrollToBottom = false;
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::SetNextItemWidth(-1.0f);
    const ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText("##ConsoleInput", m_InputBuffer, sizeof(m_InputBuffer), inputFlags, TextEditCallbackStub, this)) {
        std::string command(m_InputBuffer);
        if (!command.empty()) {
            ExecuteCommand(command);
        }
        m_InputBuffer[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

void ConsolePanel::ExecuteCommand(const std::string& commandLine) {
    m_CommandHistory.push_back(commandLine);
    m_HistoryCursor = -1;
    m_ScrollToBottom = true;

    PIECE_CORE_INFO("> {}", commandLine);

    if (commandLine == "help") {
        PIECE_CORE_INFO("Available commands: help, clear, scene.info, quit");
    } else if (commandLine == "clear") {
        EditorConsoleSink::Clear();
    } else if (commandLine == "scene.info") {
        Ref<Scene> scene = World::GetActiveScene();
        PIECE_CORE_INFO("Active scene entity count: {}", scene ? scene->GetEntityCount() : 0);
    } else if (commandLine == "quit") {
        Application::Get().Close();
    } else {
        PIECE_CORE_WARN("Unknown command '{}'. Type 'help' for a list.", commandLine);
    }
}

int ConsolePanel::TextEditCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag != ImGuiInputTextFlags_CallbackHistory) {
        return 0;
    }

    if (m_CommandHistory.empty()) {
        return 0;
    }

    if (data->EventKey == ImGuiKey_UpArrow) {
        if (m_HistoryCursor == -1) {
            m_HistoryCursor = static_cast<int>(m_CommandHistory.size()) - 1;
        } else if (m_HistoryCursor > 0) {
            --m_HistoryCursor;
        }
    } else if (data->EventKey == ImGuiKey_DownArrow) {
        if (m_HistoryCursor != -1 && ++m_HistoryCursor >= static_cast<int>(m_CommandHistory.size())) {
            m_HistoryCursor = -1;
        }
    }

    const std::string historyText = m_HistoryCursor == -1 ? std::string() : m_CommandHistory[static_cast<size_t>(m_HistoryCursor)];
    data->DeleteChars(0, data->BufTextLen);
    data->InsertChars(0, historyText.c_str());
    return 0;
}

} // namespace Piece
