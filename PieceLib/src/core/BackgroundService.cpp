#include <PiecePCH.h>

#include <core/BackgroundService.h>
#include <core/Log.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace Piece {

namespace {

std::thread                           s_Thread;
std::queue<std::function<void()>>     s_TaskQueue;
std::mutex                            s_TaskMutex;
std::condition_variable               s_TaskCV;
std::atomic<bool>                     s_Running{false};

std::vector<std::function<void()>>    s_MainCallbacks;
std::mutex                            s_MainMutex;

}

void BackgroundService::Init() {
    s_Running = true;
    s_Thread = std::thread([]() {
        PIECE_CORE_TRACE("BackgroundService: Worker thread started");
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(s_TaskMutex);
                s_TaskCV.wait(lock, [] { return !s_TaskQueue.empty() || !s_Running.load(); });
                if (!s_Running.load() && s_TaskQueue.empty()) {
                    break;
                }
                task = std::move(s_TaskQueue.front());
                s_TaskQueue.pop();
            }
            if (task) {
                task();
            }
        }
        PIECE_CORE_TRACE("BackgroundService: worker thread stopped");
    });
}

void BackgroundService::Shutdown() {
#ifdef PLATFORM_WINDOWS
    if (s_Thread.joinable()) {
        DWORD threadId = GetThreadId(static_cast<HANDLE>(s_Thread.native_handle()));
        EnumThreadWindows(threadId, [](HWND hwnd, LPARAM) -> BOOL {
            char className[64] = {};
            GetClassNameA(hwnd, className, sizeof(className));
            if (strcmp(className, "#32770") == 0) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            return TRUE;
        }, 0);
    }
#endif

    s_Running = false;
    s_TaskCV.notify_all();
    if (s_Thread.joinable()) {
        s_Thread.join();
    }
}

void BackgroundService::Submit(std::function<void()> task) {
    if (!s_Running.load()) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(s_TaskMutex);
        s_TaskQueue.push(std::move(task));
    }
    s_TaskCV.notify_one();
}

void BackgroundService::PostToMainThread(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(s_MainMutex);
    s_MainCallbacks.push_back(std::move(callback));
}

void BackgroundService::FlushMainThreadCallbacks() {
    std::vector<std::function<void()>> pending;
    {
        std::lock_guard<std::mutex> lock(s_MainMutex);
        pending = std::move(s_MainCallbacks);
        s_MainCallbacks.clear();
    }
    for (auto& cb : pending) {
        cb();
    }
}

} // namespace Piece
