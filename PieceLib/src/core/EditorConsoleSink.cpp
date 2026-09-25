#include <PiecePCH.h>

#include "EditorConsoleSink.h"

#pragma warning(push, 0)
#include <spdlog/fmt/fmt.h>
#pragma warning(pop)

namespace Piece {

std::mutex EditorConsoleSink::s_EntriesMutex;
std::deque<ConsoleLogEntry> EditorConsoleSink::s_Entries;

void EditorConsoleSink::sink_it_(const spdlog::details::log_msg& msg) {
    spdlog::memory_buf_t formatted;
    formatter_->format(msg, formatted);

    std::lock_guard<std::mutex> lock(s_EntriesMutex);
    s_Entries.push_back(ConsoleLogEntry{static_cast<int>(msg.level), fmt::to_string(formatted)});
    while (s_Entries.size() > kMaxEntries) {
        s_Entries.pop_front();
    }
}

std::vector<ConsoleLogEntry> EditorConsoleSink::GetEntries() {
    std::lock_guard<std::mutex> lock(s_EntriesMutex);
    return std::vector<ConsoleLogEntry>(s_Entries.begin(), s_Entries.end());
}

void EditorConsoleSink::Clear() {
    std::lock_guard<std::mutex> lock(s_EntriesMutex);
    s_Entries.clear();
}

} // namespace Piece
