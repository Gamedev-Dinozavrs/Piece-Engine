#pragma once

#include <string>
#include <vector>

struct ImGuiInputTextCallbackData;

namespace Piece {

// In-editor developer console: shows every engine log line and accepts simple built-in commands.
class ConsolePanel {
public:
    void OnImGuiRender();
    int TextEditCallback(ImGuiInputTextCallbackData* data);

private:
    void ExecuteCommand(const std::string& commandLine);

    char m_InputBuffer[256] = {};
    std::vector<std::string> m_CommandHistory;
    int m_HistoryCursor = -1;
    bool m_ScrollToBottom = false;
};

} // namespace Piece
