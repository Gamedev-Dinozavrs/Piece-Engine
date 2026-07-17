#pragma once

#include <core/Application.h>
#include <core/Log.h>

extern Piece::Application* Piece::CreateApplication();

#ifdef PLATFORM_WINDOWS

int main(int argc, char** argv) {
	Piece::Log::init();
	PIECE_CORE_INFO("Running Piece on Windows");
	auto app = Piece::CreateApplication();
	app->Run();
	delete app;
}

#elif PLATFORM_LINUX

int main(int argc, char** argv) {
	Piece::Log::init();
	PIECE_CORE_INFO("Running Piece on Linux");
}

#elif PLATFORM_MAC

int main(int argc, char** argv) {
	Piece::Log::init();
	PIECE_CORE_INFO("Running Piece on Mac");
}

#endif
