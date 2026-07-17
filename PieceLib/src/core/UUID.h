#pragma once

#include <xhash>

namespace Piece {

	class UUID {
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() const { return m_UUID; }
	private:
		uint64_t m_UUID;
	};

} // namespace Piece

namespace std {

	template<>
	struct hash<Piece::UUID> {
		std::size_t operator()(const Piece::UUID& uuid) const {
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};

}