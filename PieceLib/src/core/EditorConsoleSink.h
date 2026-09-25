#pragma once

#pragma warning(push, 0)
#include <spdlog/sinks/base_sink.h>
#pragma warning(pop)

#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace Piece {

struct ConsoleLogEntry {
    int level = 0; // matches spdlog::level::level_enum
    std::string message;
};

// Captures every log line written through spdlog (both core and client loggers) into a bounded
// ring buffer so the editor's Console panel can display them without depending on redirected stdout.
class EditorConsoleSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    static std::vector<ConsoleLogEntry> GetEntries();
    static void Clear();

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override {}

private:
    static std::mutex s_EntriesMutex;
    static std::deque<ConsoleLogEntry> s_Entries;
    static constexpr size_t kMaxEntries = 2000;
};

} // namespace Piece
