#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cmath>
#include <string>
#include <sstream>
#include <stdexcept>
#include <array>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

// #include "src/core/Log.h" // TODO:

// #include "src/debug/Instrumentor.h" // TODO:

#ifdef PIECE_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
