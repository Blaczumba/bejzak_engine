#include "model_loader.h"

IndexType getMatchingIndexType(size_t indicesCount) {
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
