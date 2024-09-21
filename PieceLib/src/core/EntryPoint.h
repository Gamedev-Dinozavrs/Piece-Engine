#pragma once

#include <iostream>

extern Piece::Application* Piece::CreateApplication();

#ifdef PLATFORM_WINDOWS

int main(int argc, char** argv) {
	std::cout << "Running Piece on Windows!\n";
	auto app = Piece::CreateApplication();
	app->Run();
	delete app;
}

#elif PLATFORM_LINUX

int main(int argc, char** argv) {
	std::cout << "Running Piece on Linux!\n";
}

#elif PLATFORM_MAC

int main(int argc, char** argv) {
	std::cout << Running Piece on "Mac!\n";
}

#endif
