#pragma once

#include <iostream>

extern Piece::Application* Piece::CreateApplication();

#ifdef PLATFORM_WINDOWS

int main(int argc, char** argv) {
	std::cout << "Windows!\n";
}

#elif PLATFORM_LINUX

int main(int argc, char** argv) {
	std::cout << "Linux!\n";
}

#elif PLATFORM_MAC

int main(int argc, char** argv) {
	std::cout << "Mac!\n";
}

#endif
