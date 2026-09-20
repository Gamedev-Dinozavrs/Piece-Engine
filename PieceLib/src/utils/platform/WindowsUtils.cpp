#include <PiecePCH.h>

#include <utils/platform/WindowsUtils.h>
#include <core/BackgroundService.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "Comdlg32.lib")
#endif

namespace Piece::Platform {

namespace {

std::string CopyWindowsFilter(const char* filter) {
    if (filter == nullptr) {
        return {};
    }

    size_t length = 0;
    while (filter[length] != '\0' || filter[length + 1] != '\0') {
        ++length;
    }
    length += 2;
    return std::string(filter, length);
}

} // namespace

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
    BackgroundService::Submit([filter = CopyWindowsFilter(filter), onResult = std::move(onResult)]() {
        std::string result = OpenFileDialog(filter.c_str());
        if (!result.empty()) {
            BackgroundService::PostToMainThread([result = std::move(result), onResult = std::move(onResult)]() {
                onResult(result);
            });
        }
    });
}

std::string SaveFileDialog(const char* filter, const char* defaultExtension) {
#ifdef PLATFORM_WINDOWS
    OPENFILENAMEA ofn{};
    char filePath[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = defaultExtension;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&ofn) == TRUE) {
        return std::string(filePath);
    }
#endif
    return {};
}

} // namespace Piece::Platform
