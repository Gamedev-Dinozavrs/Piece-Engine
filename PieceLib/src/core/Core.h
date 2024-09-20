#pragma once

#include <memory>

#ifdef PIECE_ENABLE_ASSERTS
	#define PIECE_ASSERT(x, ...) { if(!(x)) { PIECE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define PIECE_CORE_ASSERT(x, ...) { if(!(x)) { PIECE_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define PIECE_ASSERT(x, ...)
	#define PIECE_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define PIECE_BIND_EVENT_FUNC(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }

namespace Piece {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

} // namespace Piece