#pragma once

#include <functional>

namespace Piece {

class BackgroundService {
public:
    static void Init();
    static void Shutdown();

    // Submit a task to run on the background thread
    static void Submit(std::function<void()> task);

    // Post a callback to execute on the main thread
    // Safe to call from the background thread or any other thread
    static void PostToMainThread(std::function<void()> callback);

    // Execute all pending main-thread callbacks
    static void FlushMainThreadCallbacks();
};

} // namespace Piece
