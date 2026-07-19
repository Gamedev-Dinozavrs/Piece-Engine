#include <PiecePCH.h>

#include <utils/platform/WindowsUtils.h>
#include <core/BackgroundService.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "Comdlg32.lib")
#endif

namespace Piece::Platform {

std::string OpenFileDialog(const char* filter) {
#ifdef PLATFORM_WINDOWS
    OPENFILENAMEA ofn{};
    char filePath[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(filePath);
    }
#endif
    return {};
}

void OpenFileDialogAsync(const char* filter, std::function<void(std::string)> onResult) {
    BackgroundService::Submit([filter = std::string(filter), onResult = std::move(onResult)]() {
        std::string result = OpenFileDialog(filter.c_str());
        if (!result.empty()) {
            BackgroundService::PostToMainThread([result = std::move(result), onResult = std::move(onResult)]() {
                onResult(result);
            });
        }
    });
}

} // namespace Piece::Platform
