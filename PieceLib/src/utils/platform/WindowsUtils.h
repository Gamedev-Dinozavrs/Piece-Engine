#pragma once

#include <functional>
#include <string>

namespace Piece::Platform {

// Blocking: opens a file dialog on the calling thread.
std::string OpenFileDialog(const char* filter);

// Non-blocking: opens the dialog on the BackgroundService worker thread.
// onResult is called on the main thread (via FlushMainThreadCallbacks) with the
// selected path, or not called at all if the user cancelled.
void OpenFileDialogAsync(const char* filter, std::function<void(std::string)> onResult);

// Blocking: opens a save file dialog on the calling thread. Returns empty string if cancelled.
std::string SaveFileDialog(const char* filter, const char* defaultExtension);

// Blocking: opens a folder-picker dialog on the calling thread. Returns empty string if cancelled.
std::string BrowseFolderDialog(const char* title);

// Non-blocking: opens the folder picker on the BackgroundService worker thread.
void BrowseFolderDialogAsync(const char* title, std::function<void(std::string)> onResult);

// Directory containing the running executable (no trailing slash). Empty string on failure.
std::string GetExecutableDirectory();

// Opens a folder in the OS file explorer (non-blocking, fire-and-forget).
void OpenInFileExplorer(const std::string& path);

// Blocking: runs commandLine in workingDirectory, capturing combined stdout/stderr into output.
// Returns true if the process was launched (regardless of its exit code); exitCode receives its result.
bool RunProcess(const std::string& commandLine, const std::string& workingDirectory, int& exitCode, std::string& output);

// Non-blocking: runs the process on the BackgroundService worker thread. onComplete(exitCode, output)
// is invoked on the main thread (via FlushMainThreadCallbacks).
void RunProcessAsync(const std::string& commandLine, const std::string& workingDirectory,
    std::function<void(int, std::string)> onComplete);

// Blocking: locates the installed Visual Studio's devenv.exe via vswhere. Empty string if not found.
// Result is cached after the first successful lookup.
std::string FindVisualStudioExecutable();

// Fire-and-forget: launches commandLine without waiting for it to exit. Returns true if launched.
bool LaunchProcessDetached(const std::string& commandLine, const std::string& workingDirectory);

// Opens solutionPath and filePath together in Visual Studio (non-blocking). Returns true if launched.
bool OpenFileInVisualStudio(const std::string& solutionPath, const std::string& filePath);

} // namespace Piece::Platform
