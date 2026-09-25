#include <PiecePCH.h>

#include <utils/platform/WindowsUtils.h>
#include <core/BackgroundService.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Shell32.lib")
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

std::string GetExecutableDirectory() {
#ifdef PLATFORM_WINDOWS
    char pathBuffer[MAX_PATH] = { 0 };
    const DWORD length = GetModuleFileNameA(nullptr, pathBuffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        return {};
    }

    std::string path(pathBuffer, length);
    const size_t lastSlash = path.find_last_of("\\/");
    return lastSlash == std::string::npos ? std::string{} : path.substr(0, lastSlash);
#else
    return {};
#endif
}

void OpenInFileExplorer(const std::string& path) {
#ifdef PLATFORM_WINDOWS
    ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    (void)path;
#endif
}

std::string BrowseFolderDialog(const char* title) {
#ifdef PLATFORM_WINDOWS
    char pathBuffer[MAX_PATH] = { 0 };
    BROWSEINFOA info{};
    info.lpszTitle = title;
    info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST idList = SHBrowseForFolderA(&info);
    if (idList == nullptr) {
        return {};
    }

    std::string result;
    if (SHGetPathFromIDListA(idList, pathBuffer)) {
        result = pathBuffer;
    }
    CoTaskMemFree(idList);
    return result;
#else
    return {};
#endif
}

void BrowseFolderDialogAsync(const char* title, std::function<void(std::string)> onResult) {
    BackgroundService::Submit([title = std::string(title), onResult = std::move(onResult)]() {
        std::string result = BrowseFolderDialog(title.c_str());
        if (!result.empty()) {
            BackgroundService::PostToMainThread([result = std::move(result), onResult = std::move(onResult)]() {
                onResult(result);
            });
        }
    });
}

bool RunProcess(const std::string& commandLine, const std::string& workingDirectory, int& exitCode, std::string& output) {
#ifdef PLATFORM_WINDOWS
    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &securityAttributes, 0)) {
        return false;
    }
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(STARTUPINFOA);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdOutput = writePipe;
    startupInfo.hStdError = writePipe;
    startupInfo.hStdInput = nullptr;

    PROCESS_INFORMATION processInfo{};
    std::vector<char> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back('\0');

    const BOOL created = CreateProcessA(
        nullptr,
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
        &startupInfo,
        &processInfo);

    CloseHandle(writePipe);

    if (!created) {
        CloseHandle(readPipe);
        return false;
    }

    char buffer[4096];
    DWORD bytesRead = 0;
    while (ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
        output.append(buffer, bytesRead);
    }
    CloseHandle(readPipe);

    WaitForSingleObject(processInfo.hProcess, INFINITE);
    DWORD exitCodeValue = 0;
    GetExitCodeProcess(processInfo.hProcess, &exitCodeValue);
    exitCode = static_cast<int>(exitCodeValue);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    return true;
#else
    (void)commandLine;
    (void)workingDirectory;
    (void)exitCode;
    (void)output;
    return false;
#endif
}

void RunProcessAsync(const std::string& commandLine, const std::string& workingDirectory,
    std::function<void(int, std::string)> onComplete) {
    BackgroundService::Submit([commandLine, workingDirectory, onComplete = std::move(onComplete)]() {
        int exitCode = -1;
        std::string output;
        if (!RunProcess(commandLine, workingDirectory, exitCode, output)) {
            output += "\n[Failed to launch process]";
        }
        BackgroundService::PostToMainThread([exitCode, output = std::move(output), onComplete]() {
            onComplete(exitCode, output);
        });
    });
}

std::string FindVisualStudioExecutable() {
#ifdef PLATFORM_WINDOWS
    static std::string cachedPath;
    static bool attempted = false;
    if (attempted) {
        return cachedPath;
    }
    attempted = true;

    int exitCode = -1;
    std::string output;
    const std::string vswhere =
        "\"C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe\" -latest -property productPath";
    if (RunProcess(vswhere, "", exitCode, output) && exitCode == 0) {
        while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
            output.pop_back();
        }
        cachedPath = output;
    }
    return cachedPath;
#else
    return {};
#endif
}

bool LaunchProcessDetached(const std::string& commandLine, const std::string& workingDirectory) {
#ifdef PLATFORM_WINDOWS
    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(STARTUPINFOA);
    PROCESS_INFORMATION processInfo{};

    std::vector<char> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back('\0');

    const BOOL created = CreateProcessA(
        nullptr,
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
        &startupInfo,
        &processInfo);

    if (!created) {
        return false;
    }

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    return true;
#else
    (void)commandLine;
    (void)workingDirectory;
    return false;
#endif
}

bool OpenFileInVisualStudio(const std::string& solutionPath, const std::string& filePath) {
    const std::string devenv = FindVisualStudioExecutable();
    if (devenv.empty()) {
        return false;
    }

    const std::string commandLine = "\"" + devenv + "\" \"" + solutionPath + "\" \"" + filePath + "\"";
    return LaunchProcessDetached(commandLine, "");
}

} // namespace Piece::Platform
