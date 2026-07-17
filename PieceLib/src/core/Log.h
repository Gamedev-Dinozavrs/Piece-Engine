#pragma once

#include "Core.h"

// spdlog
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "spdlog/fmt/ostr.h"
#pragma warning(pop)

namespace Piece {
	
	class Log {
	public:
		static void init();

		inline static std::shared_ptr<spdlog::logger>& getCoreLogger() { return s_coreLogger; }
		inline static std::shared_ptr<spdlog::logger>& getClientLogger() { return s_clientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_coreLogger;
		static std::shared_ptr<spdlog::logger> s_clientLogger;
	};


} // namespace Piece


// Core log macros
#define PIECE_CORE_ERROR(...)       ::Piece::Log::getCoreLogger()->error(__VA_ARGS__)
#define PIECE_CORE_WARN(...)		::Piece::Log::getCoreLogger()->warn(__VA_ARGS__)
#define PIECE_CORE_INFO(...)		::Piece::Log::getCoreLogger()->info(__VA_ARGS__)
#define PIECE_CORE_TRACE(...)	    ::Piece::Log::getCoreLogger()->trace(__VA_ARGS__)
#define PIECE_CORE_CRITICAL(...)    ::Piece::Log::getCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define PIECE_ERROR(...)			::Piece::Log::getClientLogger()->error(__VA_ARGS__)
#define PIECE_WARN(...)			    ::Piece::Log::getClientLogger()->warn(__VA_ARGS__)
#define PIECE_INFO(...)			    ::Piece::Log::getClientLogger()->info(__VA_ARGS__)
#define PIECE_TRACE(...)			::Piece::Log::getClientLogger()->trace(__VA_ARGS__)
#define PIECE_CRITICAL(...)		    ::Piece::Log::getClientLogger()->critical(__VA_ARGS__)