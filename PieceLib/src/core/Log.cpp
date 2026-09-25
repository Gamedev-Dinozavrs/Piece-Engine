#include <PiecePCH.h>
#include "Log.h"

#include "EditorConsoleSink.h"

namespace Piece {
	std::shared_ptr<spdlog::logger> Log::s_coreLogger;
	std::shared_ptr<spdlog::logger>	Log::s_clientLogger;

	void Log::init() {
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_coreLogger = spdlog::stdout_color_mt("Piece");
		s_coreLogger->set_level(spdlog::level::trace);
		s_coreLogger->flush_on(spdlog::level::trace);
		s_clientLogger = spdlog::stdout_color_mt("PieceEditor");
		s_clientLogger->set_level(spdlog::level::trace);
		s_clientLogger->flush_on(spdlog::level::trace);

		auto consoleSink = std::make_shared<EditorConsoleSink>();
		s_coreLogger->sinks().push_back(consoleSink);
		s_clientLogger->sinks().push_back(consoleSink);
	}


} // namespace Piece