#pragma once

#include <cstdint>
#include <limits>

enum class IndexType : uint8_t {
	NONE = 0,
	UINT8 = 1,
	UINT16 = 2,
	UINT32 = 4
};

constexpr IndexType getMatchingIndexType(size_t indicesCount) {
	if (indicesCount <= std::numeric_limits<uint8_t>::max()) {
		return IndexType::UINT8;
	}
	else if (indicesCount <= std::numeric_limits<uint16_t>::max()) {
		return IndexType::UINT16;
	}
	else if (indicesCount <= std::numeric_limits<uint32_t>::max()) {
		return IndexType::UINT32;
	}
	return IndexType::NONE;
}