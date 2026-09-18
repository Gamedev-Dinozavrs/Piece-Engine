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

} // namespace Piece::Platform
